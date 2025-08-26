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
	}
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

struct cmd *parse_inputs(int argc, char *argv[])
{
	if(argc > MAX_SUB_CMDS)
	{
		lal_error(ERROR_INPUT_OVERFLOW);
	}

	struct cmd *cmd = malloc(sizeof(struct cmd));
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

		if(argv[i][0] == '-')
		{
			cmd->sub_cmds[i - 1].type = FLAG;
		}
		else 
		{
			cmd->sub_cmds[i - 1].type = INPUT;
		}

		for(int j = 0; j < strlen(argv[i]); j++)
		{
			if(char_v_append(cmd->sub_cmds[i - 1].contents, argv[i][j]) == FALSE)
			{
				lal_error(ERROR_FAILED_RESIZE);
			}
		}
	}

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

bool safe_compare(char *buf, int index, int len, off_t size, const char *str)
{
	if(index + len < size)
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

	while(contents[*index] == ':')
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
		label->components_len++;
		label->components[label->components_len - 1].type = LAL_ARG;
		label->components[label->components_len - 1].contents = init_char_v();
		int depth = 0;

		while(depth > 0)
		{
			if(safe_compare(contents, *index, strlen("<<"), size, "<<"))
			{
				depth++;
				*index += strlen("<<");
			}
			else if(safe_compare(contents, *index, strlen(">>"), size, ">>"))
			{
				depth--;
				*index += strlen(">>");
			}

			if(depth > 0)
			{
				char_v_append(label->components[label->components_len - 1].contents, contents[*index]);
				(*index)++;
			}

			if(*index >= size)
			{
				return 0;
			}
		}
	}
	else 
	{
		if(label->components[label->components_len - 1].type != LAL_PLAIN)
		{
			label->components_len++;
			label->components[label->components_len - 1].type = LAL_PLAIN;
			label->components[label->components_len - 1].contents = init_char_v();
		}

		char_v_append(label->components[label->components_len].contents, contents[*index]);
	}


	return 1;
}

int parse_line(alias_node *label, char *contents, int *index, off_t size)
{
	if(safe_compare(contents, *index, strlen("{"), size, "{"))
	{
		label->components[label->components_len].type = LAL_NEW_LINE;

		int depth = 1;

		while(depth > 0)
		{
			if(safe_compare(contents, *index, strlen("{"), size, "{"))
			{
				depth++;
				*index += strlen("{");
			}
			else if(safe_compare(contents, *index, strlen("{"), size, "{"))
			{
				depth--;
				*index += strlen("}");
			}

			if(depth > 0)
			{
				parse_inner(label, contents, index, size);
				(*index)++;
			}

			if(*index >= size)
			{
				return 0;
			}
		}
	}

	return 1;
}

int parse_componenets(alias_node *label, char *contents, int *index, off_t size)
{
	label->components_len = 0;

	while(!safe_compare(contents, *index, strlen("<<END>>"), size, "<<END>>"))
	{
		parse_line(label, contents, index, size);

		if(*index >= size)
		{
			return 0;
		}

		(*index)++;
	}

	label->components[label->components_len].type = LAL_END;

	return 1;
}

alias_node *process_lal_file()
{
	alias_node *labels = malloc(sizeof(struct alias_node));
	labels->components_len = 0;

	FILE *file = fopen(".lal", "r+b");

	if(!file)
	{
		file = fopen(".lal", "w+b");
	}

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
		}

		if(labels_head->components[labels_head->components_len - 1].type == LAL_END)
		{
			labels_head->next_node = malloc(sizeof(alias_node));
			labels_head = labels_head->next_node;
			labels_head->components_len = 0;
		}
	}

	print_char_v(*labels->name);

	for(int i = 0; i < labels->components_len; i++)
	{
		if(labels->components[i].type == LAL_NEW_LINE)
		{
			printf("\n");
		}
		else if(labels->components[i].type == LAL_END)
		{
			printf("\n<<END>>");
		}
		else if(labels->components[i].type == LAL_PLAIN)
		{
			print_char_v(*labels->components[i].contents);
		}
		else if(labels->components[i].type == LAL_ARG)
		{
			printf("<<");
			print_char_v(*labels->components[i].contents);
			printf(">>");
		}
	}


	return labels;
}

