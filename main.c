#include <stdio.h>

#include "lalias.h"

int main(int argc, char *argv[])
{
	struct cmd *cmd = parse_inputs(argc, argv);

	process_lal_file();

	return 0;
}
