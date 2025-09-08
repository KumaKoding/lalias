#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "lalias.h"

typedef int bool;

#define TRUE (bool)1
#define FALSE (bool)0

#define INITIAL_VECTOR_SIZE 32

enum error_code
{
	ERROR_INPUT_OVERFLOW,
	ERROR_FAILED_RESIZE,
	ERROR_UNKNOWN_FLAG,
	ERROR_NO_INPUT,
	ERROR_NO_LABEL,
	ERROR_NO_LAL,
	ERROR_FAILED_READ,
	ERROR_UNEXPECTED_EOF,
	ERROR_INVALID_CHARACTERS_IN_LABEL,
	ERROR_NO_NAME,
	ERROR_NO_COMMAND,
	ERROR_NO_FILE,
	ERROR_INSUFFICIENT_INPUTS,
	ERROR_BAD_NUMERICAL_INPUT,
	ERROR_FAILED_TO_TRUNCATE,
	ERROR_LABEL_NOT_FOUND,
	ERROR_LAL_REWRITE_FAILURE
};

void lal_error(enum error_code code)
{
	switch (code) 
	{
		case ERROR_INPUT_OVERFLOW:
			printf("ERROR: Input contains too many subcommands.\n");
			exit(1);
		case ERROR_FAILED_RESIZE:
			printf("ERROR: Failed to resize char_v, unable to complete command.\n");
			exit(1);
		case ERROR_UNKNOWN_FLAG:
			printf("ERROR: Unknown flag.\n");
			exit(1);
		case ERROR_NO_INPUT:
			printf("ERROR: No input.\n");
			exit(1);
		case ERROR_NO_LABEL:
			printf("ERROR: No label.\n");
			exit(1);
		case ERROR_NO_LAL:
			printf("ERROR: No .lal file exists.\n");
			exit(1);
		case ERROR_FAILED_READ:
			printf("ERROR: Failed to read .lal.\n");
			exit(1);
		case ERROR_UNEXPECTED_EOF:
			printf("ERROR: Unexpected END OF FILE in .lal.\n");
			exit(1);
		case ERROR_INVALID_CHARACTERS_IN_LABEL:
			printf("ERROR: Restricted characters in label(s) in .lal.\n");
			exit(1);	
		case ERROR_NO_NAME:
			printf("ERROR: Error occurred when parsing rule name.\n");
			exit(1);
		case ERROR_NO_COMMAND:
			printf("ERROR: Error occurred when parsing rule command.\n");
			exit(1);
		case ERROR_NO_FILE:
			printf("ERROR: File inputted not found.\n");
			exit(1);
		case ERROR_INSUFFICIENT_INPUTS:
			printf("ERROR: Insufficient amount of inputs.\n");
			exit(1);
		case ERROR_BAD_NUMERICAL_INPUT:
			printf("ERROR: Unexpected characters in positive integer input.\n");
			exit(1);
		case ERROR_FAILED_TO_TRUNCATE:
			printf("ERROR: Failed to truncate label, insufficient or improperly formatted lines.\n");
			exit(1);
		case ERROR_LABEL_NOT_FOUND:
			printf("ERROR: Inputted label not found.\n");
			exit(1);
		case ERROR_LAL_REWRITE_FAILURE:
			printf("ERROR: Unexpected issues during rewrite of .lal file.\n");
			exit(1);
	}
}

int nn_int_from_str(char *str, int len)
{
	int n = 0;

	for(int i = 0; i < len; i++)
	{
		if(str[i] < '0' || str[i] > '9')
		{
			return -1;
		}

		n *= 10;
		n += str[i] - '0';
	}

	return n;
}

bool safe_compare(char *buf, int index, int len, off_t size, const char *str)
{
	if(index + len > size)
	{
		return FALSE;
	}

	for(int i = 0; i < len; i++)
	{
		if(buf[index + i] != str[i])
		{
			return FALSE;
		}
	}

	return TRUE;
}

bool compare_char_v(char_v *v1, char_v *v2)
{
	if(v1->len != v2->len)
	{
		return FALSE;
	}

	for(int i = 0; i < v1->len; i++)
	{
		if(v1->data[i] != v2->data[i])
		{
			return FALSE;
		}
	}

	return TRUE;
}

char_v *init_char_v()
{
	char_v *vector = malloc(sizeof(char_v));

	vector->data = malloc(INITIAL_VECTOR_SIZE);
	vector->max = INITIAL_VECTOR_SIZE;
	vector->len = 0;

	return vector;
}

void free_char_v(char_v *v)
{
	free(v->data);
	free(v);
}

int char_v_append(char_v *vec, char c)
{
	if(vec->len >= vec->max)
	{
		vec->data = realloc(vec->data, vec->max * 2 * sizeof(char));
		vec->max *= 2;

		if(!vec->data)
		{
			return 0;
		}
	}

	vec->data[vec->len] = c;
	vec->len++;

	return 1;
}

char_v *copy_char_v(char_v *v)
{
	char_v *copy = init_char_v();

	for(int i = 0; i < v->len; i++)
	{
		if(char_v_append(copy, v->data[i]) == 0)
		{
			lal_error(ERROR_FAILED_RESIZE);
		}
	}

	return copy;
}

void print_char_v(char_v v)
{
	for(int i = 0; i < v.len; i++)
	{
		printf("%c", v.data[i]);
	}
}

void print_nodes(alias_node *nodes)
{
	for(alias_node *node = nodes; node != NULL; node = node->next_node)
	{
		print_char_v(*node->name);

		for(int i = 0; i < node->components_len; i++)
		{
			if(node->components[i].type == LAL_NEW_LINE)
			{
				printf("\n");
			}
			else if(node->components[i].type == LAL_END)
			{
				printf("\n<<END>>");
			}
			else if(node->components[i].type == LAL_PLAIN)
			{
				print_char_v(*node->components[i].contents);
			}
			else if(node->components[i].type == LAL_ARG)
			{
				printf("<<");
				print_char_v(*node->components[i].contents);
				printf(">>");
			}
		}

		printf("\n");
	}
}

commands *parse_inputs(int argc, char *argv[])
{
	if(argc > MAX_SUB_CMDS)
	{
		lal_error(ERROR_INPUT_OVERFLOW);
	}

	commands *cmd = malloc(sizeof(commands));
	cmd->n_cmds = 0;

	if(argc == 1)
	{
		cmd->sub_cmds[0].type = EMPTY;
		cmd->sub_cmds[0].contents = NULL;

		return cmd;
	}

	for(int i = 1; i < argc; i++)
	{
		cmd->sub_cmds[i - 1].contents = init_char_v();
		int offset = 0;

		if(i == 1 && argv[i][0] == '-')
		{
			offset = 1;

			cmd->sub_cmds[i - 1].type = FLAG;
		}
		else 
		{
			offset = 0;
			cmd->sub_cmds[i - 1].type = INPUT;
		}

		for(int j = 0; j < strlen(argv[i]) - offset; j++)
		{
			char_v_append(cmd->sub_cmds[i - 1].contents, argv[i][j + offset]);
		}

		cmd->n_cmds++;
	}

	for(int i = 0; i < cmd->n_cmds; i++)
	{
		switch(cmd->sub_cmds[i].type)
		{
			case INPUT:
				printf("\"");
				print_char_v(*cmd->sub_cmds[i].contents);
				printf("\"");
				break;
			case FLAG:
				printf("-");
				print_char_v(*cmd->sub_cmds[i].contents);
				break;
			case EMPTY:
				printf("<<EMPTY>>");
				break;
		}

		printf(" ");
	}

	printf("\n");

	return cmd;
}

off_t fsize(const char *file_name)
{
	struct stat s;
	int e = stat(file_name, &s);

	if(e == 0)
	{
		return s.st_size;
	}

	lal_error(ERROR_FAILED_READ);

	return -1;
}

bool is_restricted(char c)
{
	for(int i = 0; i < strlen(RESTRICTED_NAME_CHARACTERS); i++)
	{
		if(c == RESTRICTED_NAME_CHARACTERS[i])
		{
			return TRUE;
		}
	}

	return FALSE;
}

int parse_name(alias_node *label, char *contents, int *index, off_t size)
{
	label->name = init_char_v();

	while(contents[*index] != ':')
	{
		if(is_restricted(contents[*index]))
		{
			lal_error(ERROR_INVALID_CHARACTERS_IN_LABEL);
		}

		if(*index >= size)
		{
			return 0;
		}

		char_v_append(label->name, contents[*index]);

		(*index)++;
	}

	return 1;
}

int parse_inner(alias_node *label, char *contents, int *index, off_t size)
{
	if(safe_compare(contents, *index, strlen("<<"), size, "<<"))
	{
		*index += strlen("<<");

		label->components_len++;
		label->components[label->components_len - 1].type = LAL_ARG;
		label->components[label->components_len - 1].contents = init_char_v();
		int depth = 1;

		while(depth > 0)
		{
			int jump = 1;

			if(safe_compare(contents, *index, strlen("<<"), size, "<<"))
			{
				depth++;
				jump = strlen("<<");
			}
			else if(safe_compare(contents, *index, strlen(">>"), size, ">>"))
			{
				depth--;
				jump = strlen(">>");
			}

			if(depth > 0)
			{
				char_v_append(label->components[label->components_len - 1].contents, contents[*index]);
			}

			*index += jump;

			if(*index >= size)
			{
				return 0;
			}
		}
	}
	else 
	{
		if(label->components_len == 0 || label->components[label->components_len - 1].type != LAL_PLAIN)
		{
			label->components_len++;
			label->components[label->components_len - 1].type = LAL_PLAIN;
			label->components[label->components_len - 1].contents = init_char_v();
		}

		char_v_append(label->components[label->components_len - 1].contents, contents[*index]);
		(*index)++;
	}

	return 1;
}

int parse_line(alias_node *label, char *contents, int *index, off_t size)
{
	if(safe_compare(contents, *index, strlen("{"), size, "{"))
	{
		*index += strlen("{");

		label->components[label->components_len].type = LAL_NEW_LINE;
		label->components[label->components_len].contents = NULL;
		label->components_len++;

		int depth = 1;

		while(depth > 0)
		{
			if(safe_compare(contents, *index, strlen("{"), size, "{"))
			{
				depth++;
				(*index)++;
			}
			else if(safe_compare(contents, *index, strlen("}"), size, "}"))
			{
				depth--;
				(*index)++;
			}

			if(depth > 0)
			{
				if(parse_inner(label, contents, index, size) == 0)
				{
					lal_error(ERROR_NO_COMMAND);
				}
			}

			if(*index >= size)
			{
				return 0;
			}
		}

		label->components[label->components_len].type = LAL_END_LINE;
		label->components[label->components_len].contents = NULL;
		label->components_len++;
	}
	else 
	{
		(*index)++;
	}

	return 1;
}

int parse_components(alias_node *label, char *contents, int *index, off_t size)
{
	label->components_len = 0;

	while(!safe_compare(contents, *index, strlen("<<END>>"), size, "<<END>>"))
	{
		if(parse_line(label, contents, index, size) == 0)
		{
			lal_error(ERROR_NO_COMMAND);
		}

		if(*index >= size)
		{
			return 0;
		}
	}

	*index += strlen("<<END>>");

	label->components[label->components_len].type = LAL_END;
	label->components_len++;

	while(is_restricted(contents[*index]))
	{
		(*index)++;
	}

	return 1;
}

FILE *open_lal()
{
	FILE *file = fopen(".lal", "r+b");

	if(!file)
	{
		file = fopen(".lal", "w+b");
	}

	return file;
}

alias_node *process_lal_file(FILE *file)
{
	alias_node *labels = malloc(sizeof(struct alias_node));
	labels->next_node = NULL;
	labels->components_len = 0;

	off_t size = fsize(".lal");
	char *contents = malloc(size * sizeof(char));

	size_t e = fread(contents, sizeof(char), size, file);

	if(e != size)
	{
		if(feof(file))
		{
			lal_error(ERROR_UNEXPECTED_EOF);
		}
		else if(ferror(file))
		{
			lal_error(ERROR_FAILED_READ);
		}
	}

	int c = 0;

	printf("%lld\n\n", size);

	if(size == 0)
	{
		return NULL;
	}

	alias_node *labels_head = labels;

	while (c < size) 
	{
		if(parse_name(labels_head, contents, &c, size) == 0)
		{
			lal_error(ERROR_NO_NAME);
		}

		if(parse_components(labels_head, contents, &c, size) == 0)
		{
			lal_error(ERROR_NO_COMMAND);
		}

		if(c < size && labels_head->components[labels_head->components_len - 1].type == LAL_END)
		{
			labels_head->next_node = malloc(sizeof(alias_node));
			labels_head = labels_head->next_node;
			labels_head->components_len = 0;
			labels_head->next_node = NULL;
		}
	}

	free(contents);

	return labels;
}

bool exact_match(char *str1, int len1, char *str2, int len2)
{
	if(len1 != len2)
	{
		return FALSE;
	}

	if(!safe_compare(str1, 0, len2, len1, str2))
	{
		return FALSE;
	}

	return TRUE;
}

void char_v_append_char_v(char_v *targ, char_v *appd)
{
	for(int i = 0; i < appd->len; i++)
	{
		if(char_v_append(targ, appd->data[i]) == 0)
		{
			lal_error(ERROR_FAILED_RESIZE);
		}
	}
}

void char_v_append_str(char_v *targ, const char *appd)
{
	for(int i = 0; i < strlen(appd); i++)
	{
		if(char_v_append(targ, appd[i]) == 0)
		{
			lal_error(ERROR_FAILED_RESIZE);
		}
	}
}

void reconstruct_lal(char_v *lal, alias_node *label)
{
	for(alias_node *node = label; node != NULL; node = node->next_node)
	{
		char_v_append_char_v(lal, node->name);
		if(char_v_append(lal, ':') == 0)
		{
			lal_error(ERROR_FAILED_RESIZE);
		}

		for(int i = 0; i < node->components_len; i++)
		{
			switch (node->components[i].type) 
			{
				case LAL_PLAIN:
					char_v_append_char_v(lal, node->components[i].contents);
					break;
				case LAL_ARG:
					char_v_append_str(lal, "<<");
					char_v_append_char_v(lal, node->components[i].contents);
					char_v_append_str(lal, ">>");
					break;
				case LAL_NEW_LINE:
					if(char_v_append(lal, '{') == 0)
					{
						lal_error(ERROR_FAILED_RESIZE);
					}
					break;
				case LAL_END_LINE:
					if(char_v_append(lal, '}') == 0)
					{
						lal_error(ERROR_FAILED_RESIZE);
					}
					break;
				case LAL_END:
					char_v_append_str(lal, "<<END>>\n");
					break;
			}
		}
	}
}

void reset_component(struct alias_components *component)
{
	switch (component->type) 
	{
		case LAL_PLAIN:
			free_char_v(component->contents);
			break;
		case LAL_ARG:
			free_char_v(component->contents);
			break;
		case LAL_NEW_LINE:
			break;
		case LAL_END_LINE:
			break;
		case LAL_END:
			break;
	}

	component->contents = NULL;
	component->type = 0;
}

void delete_components(struct alias_components *components, int n_components)
{
	for(int i = 0; i < n_components; i++)
	{
		reset_component(&components[i]);
	}
}

void delete_node(alias_node *node, alias_node *prev, alias_node **head)
{
	if(prev== NULL && node->next_node == NULL)
	{
		alias_node *tmp = node;
		*head = NULL;
		free_char_v(tmp->name);
		delete_components(tmp->components, tmp->components_len);
		free(tmp);
	}
	else if(node->next_node == NULL)
	{
		alias_node *tmp = node;
		node = NULL;
		prev->next_node = NULL;
		free_char_v(tmp->name);
		delete_components(tmp->components, tmp->components_len);
		free(tmp);
	}
	else if(prev== NULL)
	{
		alias_node *tmp = node;
		*head = node->next_node;
		node = NULL;
		free_char_v(tmp->name);
		delete_components(tmp->components, tmp->components_len);
		free(tmp);
	}
	else 
	{
		prev->next_node = node->next_node;
		free_char_v(node->name);
		delete_components(node->components, node->components_len);
		free(node);
	}
}

#define FLAGS_APPEND_NAME_OFFSET 1
#define FLAGS_APPEND_INPUT_OFFSET 2
#define FLAGS_APPEND_MIN_SUBCMDS 3

#define FLAGS_TRUNCATE_NAME_OFFSET 1
#define FLAGS_TRUNCATE_NUMBER_OFFSET 2
#define FLAGS_TRUNCATE_MIN_SUBCMDS 2
#define FLAGS_TRUNCATE_DEFAULT_SUBCMDS 2

#define FLAGS_DELETE_NAME_OFFSET 1
#define FLAGS_DELETE_MIN_SUBCMDS 2

#define FLAGS_RENAME_NAME_OFFSET 1
#define FLAGS_RENAME_INPUT_OFFSET 2
#define FLAGS_RENAME_MIN_SUBCMDS 3

void append_to_lal(commands *cmd, alias_node **labels)
{
	if(cmd->n_cmds < FLAGS_APPEND_MIN_SUBCMDS)
	{
		lal_error(ERROR_INSUFFICIENT_INPUTS);
	}

	bool is_new = TRUE;
	alias_node *current_node = NULL;
	alias_node *last_node = NULL;

	char_v *name = cmd->sub_cmds[FLAGS_APPEND_NAME_OFFSET].contents;

	for(alias_node *node = *labels; node != NULL; node = node->next_node)
	{
		last_node = node;

		if(compare_char_v(name, node->name))
		{
			is_new = FALSE;

			current_node = node;

			break;
		}
	}

	if(is_new)
	{
		if(last_node)
		{
			last_node->next_node = malloc(sizeof(alias_node));
			current_node = last_node->next_node;
		}
		else 
		{
			*labels = malloc(sizeof(alias_node));
			current_node = *labels;
		}

		current_node->name = copy_char_v(name); 
		current_node->components_len = 0;
	}
	else 
	{
		// to replace <<END>> with a LAL_NEW_LINE
		reset_component(&current_node->components[current_node->components_len - 1]);
		current_node->components_len--;
	}

	for(int sc = FLAGS_APPEND_INPUT_OFFSET; sc < cmd->n_cmds; sc++)
	{
		current_node->components_len++;
		current_node->components[current_node->components_len - 1].type = LAL_NEW_LINE;

		int i = 0;

		while(i < cmd->sub_cmds[sc].contents->len)
		{
			parse_inner(current_node, cmd->sub_cmds[sc].contents->data, &i, cmd->sub_cmds[sc].contents->len);
		}

		current_node->components_len++;
		current_node->components[current_node->components_len - 1].type = LAL_END_LINE;
	}

	current_node->components_len++;
	current_node->components[current_node->components_len - 1].type = LAL_END;
}

void truncate_from_lal(commands *cmd, alias_node **labels)
{
	if(cmd->n_cmds < FLAGS_TRUNCATE_MIN_SUBCMDS)
	{
		lal_error(ERROR_INSUFFICIENT_INPUTS);
	}

	alias_node *current_node = NULL;
	alias_node *prev_node = NULL;

	char_v *name = cmd->sub_cmds[FLAGS_TRUNCATE_NAME_OFFSET].contents;

	for(alias_node *node = *labels; node != NULL; node = node->next_node)
	{
		if(compare_char_v(node->name, name))
		{
			current_node = node;

			break;
		}
		else 
		{
			prev_node = node;
		}
	}

	if(current_node == NULL)
	{
		lal_error(ERROR_LABEL_NOT_FOUND);
	}

	int n_truncate = 1;

	if(cmd->n_cmds > FLAGS_TRUNCATE_DEFAULT_SUBCMDS)
	{
		struct sub_cmd number_input = cmd->sub_cmds[FLAGS_TRUNCATE_NUMBER_OFFSET];
		n_truncate = nn_int_from_str(number_input.contents->data, number_input.contents->len);

		if(n_truncate < 0)
		{
			lal_error(ERROR_BAD_NUMERICAL_INPUT);
		}
	}

	for(int n = 0; n < n_truncate; n++)
	{
		while(current_node->components_len > 1 && current_node->components[current_node->components_len - 1].type != LAL_NEW_LINE)
		{
			reset_component(&current_node->components[current_node->components_len - 1]);
			current_node->components_len--;
		}

		current_node->components[current_node->components_len - 1].type = LAL_END;
		current_node->components[current_node->components_len - 1].contents = NULL;
	}

	if(current_node->components_len == 1)
	{
		delete_node(current_node, prev_node, labels);
	}
}

void delete_from_lal(commands *cmd, alias_node **labels)
{
	if(cmd->n_cmds < FLAGS_DELETE_MIN_SUBCMDS)
	{
		lal_error(ERROR_INSUFFICIENT_INPUTS);
	}

	alias_node *current_node = NULL;
	alias_node *prev_node = NULL;

	char_v *name = cmd->sub_cmds[FLAGS_DELETE_NAME_OFFSET].contents;

	for(alias_node *node = *labels; node != NULL; node = node->next_node)
	{
		if(compare_char_v(node->name, name))
		{
			current_node = node;

			break;
		}
		else 
		{
			prev_node = node;
		}
	}

	if(current_node == NULL)
	{
		lal_error(ERROR_LABEL_NOT_FOUND);
	}

	delete_node(current_node, prev_node, labels);
}

void rename_in_lal(commands *cmd, alias_node *labels)
{
	if(cmd->n_cmds < FLAGS_RENAME_MIN_SUBCMDS)
	{
		lal_error(ERROR_INSUFFICIENT_INPUTS);
	}

	alias_node *current_node = NULL;

	char_v *name = cmd->sub_cmds[FLAGS_RENAME_NAME_OFFSET].contents;

	for(alias_node *node = labels; node != NULL; node = node->next_node)
	{
		if(compare_char_v(node->name, name))
		{
			current_node = node;

			break;
		}
	}

	if(current_node == NULL)
	{
		lal_error(ERROR_LABEL_NOT_FOUND);
	}

	current_node->name = copy_char_v(cmd->sub_cmds[FLAGS_RENAME_INPUT_OFFSET].contents);
}

int use_flags(commands *cmd, alias_node **labels, FILE *file)
{
	char_v *new_lal = init_char_v();
	char_v *flag = cmd->sub_cmds[0].contents;

	if(exact_match(flag->data, flag->len, "-append", strlen("-append")) || exact_match(flag->data, flag->len, "a", strlen("a")))
	{
		append_to_lal(cmd, labels);
	}
	else if(exact_match(flag->data, flag->len, "-truncate", strlen("-truncate")) || exact_match(flag->data, flag->len, "t", strlen("t")))
	{
		truncate_from_lal(cmd, labels);
	}
	else if(exact_match(flag->data, flag->len, "-delete", strlen("-delete")) || exact_match(flag->data, flag->len, "d", strlen("d")))
	{
		delete_from_lal(cmd, labels);
	}
	else if(exact_match(flag->data, flag->len, "-rename", strlen("-rename")) || exact_match(flag->data, flag->len, "rn", strlen("rn")))
	{
		rename_in_lal(cmd, *labels);
	}
	else 
	{
		lal_error(ERROR_UNKNOWN_FLAG);
	}

	reconstruct_lal(new_lal, *labels);

	FILE *overwrite = freopen(NULL, "w+b", file);

	if(overwrite == NULL)
	{
		return 0;
	}

	int write_check = fwrite(new_lal->data, sizeof(new_lal->data[0]), new_lal->len, overwrite);

	if(feof(overwrite) || write_check != new_lal->len)
	{
		return 0;
	}

	free_char_v(new_lal);

	return 1;
}

#define INPUT_NAME_OFFSET 0
#define INPUT_ARGS_OFFSET 1
#define INPUT_MIN_SUBCMDS 1

void use_input(commands *cmd, alias_node *labels)
{
	if(cmd->n_cmds < INPUT_MIN_SUBCMDS)
	{
		lal_error(ERROR_INSUFFICIENT_INPUTS);
	}

	alias_node *current_node = NULL;

	char_v *name = cmd->sub_cmds[INPUT_NAME_OFFSET].contents;

	for(alias_node *node = labels; node != NULL; node = node->next_node)
	{
		if(compare_char_v(node->name, name))
		{
			current_node = node;

			break;
		}
	}

	if(current_node == NULL)
	{
		lal_error(ERROR_LABEL_NOT_FOUND);
	}

	char_v **args = malloc(sizeof(char_v *) * cmd->n_cmds - INPUT_ARGS_OFFSET);

	for(int a = 0; a < cmd->n_cmds - INPUT_ARGS_OFFSET; a++)
	{
		args[a] = init_char_v();
		char_v_append_char_v(args[a], cmd->sub_cmds[a + INPUT_ARGS_OFFSET].contents);
	}

	char_v *sys_cmd = init_char_v();

	for(int i = 0; i < current_node->components_len; i++)
	{
		if(current_node->components[i].type == LAL_PLAIN)
		{
			char_v_append_char_v(sys_cmd, current_node->components[i].contents);
		}
		else if(current_node->components[i].type == LAL_ARG)
		{
			int arg_n = nn_int_from_str(current_node->components[i].contents->data, current_node->components[i].contents->len);

			if(arg_n >= cmd->n_cmds - INPUT_ARGS_OFFSET)
			{
				lal_error(ERROR_INSUFFICIENT_INPUTS);
			}

			char_v_append_char_v(sys_cmd, args[arg_n]);
		}
		else if(current_node->components[i].type == LAL_END_LINE)
		{
			char_v_append(sys_cmd, '\0');

			system(sys_cmd->data);
			free_char_v(sys_cmd);

			sys_cmd = init_char_v();
		}
		else if(current_node->components[i].type == LAL_END)
		{
			free_char_v(sys_cmd);
		}
	}

	for(int a = 0; a < cmd->n_cmds - INPUT_ARGS_OFFSET; a++)
	{
		free_char_v(args[a]);
	}

	free(args);
}

int use_default(alias_node *labels)
{
}

int run_command(commands *cmd, alias_node **labels, FILE *file)
{
	if(cmd->sub_cmds[0].type == FLAG)
	{
		if(use_flags(cmd, labels, file) == 0)
		{
			lal_error(ERROR_LAL_REWRITE_FAILURE);
		}
	}
	else if(cmd->sub_cmds[0].type == INPUT)
	{
		use_input(cmd, *labels);
	}
	else if(cmd->sub_cmds[0].type == EMPTY)
	{
	}

	return 1;
}

