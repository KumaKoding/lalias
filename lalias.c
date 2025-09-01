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
	ERROR_NO_COMMAND
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
	}
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

		if(argv[i][0] == '-')
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
	}
	else 
	{
		(*index)++;
	}

	return 1;
}

int parse_componenets(alias_node *label, char *contents, int *index, off_t size)
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

	alias_node *labels_head = labels;

	while (c < size) 
	{
		if(parse_name(labels_head, contents, &c, size) == 0)
		{
			lal_error(ERROR_NO_NAME);
		}

		print_char_v(*labels_head->name);
		printf("\n");

		if(parse_componenets(labels_head, contents, &c, size) == 0)
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
					if(i > 0)
					{
						if(char_v_append(lal, '}') == 0)
						{
							lal_error(ERROR_FAILED_RESIZE);
						}
					}

					if(i < node->components_len - 1)
					{
						if(char_v_append(lal, '{') == 0)
						{
							lal_error(ERROR_FAILED_RESIZE);
						}
					}
					break;
				case LAL_END:
					char_v_append_str(lal, "<<END>>");
					break;
			}
		}
	}
}

int append_to_lal(commands *cmd, alias_node *labels)
{
	bool is_new = TRUE;
	alias_node *current_node;
	alias_node *last_node;

	char_v *name = cmd->sub_cmds[1].contents;

	for(alias_node *node = labels; node != NULL; node = node->next_node)
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
		last_node->next_node = malloc(sizeof(alias_node));
		current_node = last_node->next_node;

		current_node->name = name;
		current_node->components_len = 0;
	}
	else 
	{
		// to replace <<END>> with a LAL_NEW_LINE
		current_node->components_len--;
	}

	for(int sc = 2; sc < cmd->n_cmds; sc++)
	{
		current_node->components_len++;
		current_node->components[current_node->components_len - 1].type = LAL_NEW_LINE;

		int i = 0;

		while(i < cmd->sub_cmds[sc].contents->len)
		{
			parse_inner(current_node, cmd->sub_cmds[sc].contents->data, &i, cmd->sub_cmds[sc].contents->len);
		}
	}

	current_node->components_len++;
	current_node->components[current_node->components_len - 1].type = LAL_END;

	return 1;
}

int use_flags(commands *cmd, alias_node *labels)
{
	char_v *new_lal = init_char_v();
	char_v *flag = cmd->sub_cmds[0].contents;

	if(exact_match(flag->data, flag->len, "-append", strlen("-append")) || exact_match(flag->data, flag->len, "a", strlen("a")))
	{
		append_to_lal(cmd, labels);
	}
	else if(exact_match(flag->data, flag->len, "-delete", strlen("-delete")) || exact_match(flag->data, flag->len, "d", strlen("d")))
	{
	}
	else if(exact_match(flag->data, flag->len, "-rename", strlen("-rename")) || exact_match(flag->data, flag->len, "rn", strlen("rn")))
	{
	}
	else if(exact_match(flag->data, flag->len, "-overwrite", strlen("-overwrite")) || exact_match(flag->data, flag->len, "ow", strlen("ow")))
	{
	}
	else 
	{
		lal_error(ERROR_UNKNOWN_FLAG);
	}

	reconstruct_lal(new_lal, labels);

	return 1;
}

int use_input(commands *cmd, alias_node *labels)
{
}

int use_default(alias_node *labels)
{
}

int run_command(commands *cmd, alias_node *labels, FILE *file)
{
	if(cmd->sub_cmds[0].type == FLAG)
	{
		if(use_flags(cmd, labels) == 0)
		{

		}
	}
	else if(cmd->sub_cmds[0].type == INPUT)
	{
	}
	else if(cmd->sub_cmds[0].type == EMPTY)
	{
	}

	return 1;
}

