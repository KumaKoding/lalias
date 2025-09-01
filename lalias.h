#include <stdio.h>
#include <stddef.h>
#define MAX_SUB_CMDS 128
#define MAX_ALIAS_COMPONENTS 256
#define RESTRICTED_NAME_CHARACTERS " \n{}<>"

typedef struct char_v char_v;
typedef struct alias_node alias_node;
typedef struct commands commands;

enum sub_cmd_type 
{
	INPUT,
	FLAG, // append, rename, replace, delete, add
	EMPTY
};

enum alias_type
{
	LAL_PLAIN,
	LAL_ARG,
	LAL_NEW_LINE,
	LAL_END,
};

struct sub_cmd
{
	enum sub_cmd_type type;
	char_v *contents;
};

struct commands
{
	struct sub_cmd sub_cmds[MAX_SUB_CMDS];
	int n_cmds;
};

struct char_v
{
	char *data;
	int max;
	int len;
};

struct alias_components
{
	enum alias_type type;
	char_v *contents;
};

struct alias_node
{
	struct alias_components components[MAX_ALIAS_COMPONENTS];
	char_v *name;
	int components_len;
	alias_node *next_node;
};

commands *parse_inputs(int argc, char *argv[]);
alias_node *process_lal_file(FILE *file);
int run_command(commands *cmd, struct alias_node *labels, FILE *file);
FILE *open_lal();
void print_nodes(alias_node *nodes);

