#include <stdio.h>

#include "lalias.h"

int main(int argc, char *argv[])
{
	FILE *lal = open_lal();

	commands *cmds = parse_inputs(argc, argv);
	alias_node *nodes = process_lal_file(lal);

	run_command(cmds, &nodes, lal);

	// free cmd
	// free nodes
	

	fclose(lal);

	// print_nodes(nodes);

	return 0;
}
