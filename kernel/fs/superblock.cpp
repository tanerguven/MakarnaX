
#include "vfs.h"

#include <string.h>
#include "../kernel.h"
#include "denemefs/denemefs.h"

#define NR_SUPERBLOCKS 32


static struct SuperBlock superblocks[NR_SUPERBLOCKS];

void mount_root(Task *init_task) {

	memset(superblocks, 0, sizeof(superblocks));
	superblocks[0].dev = 123;
	superblocks[0].fs_type = 123;

	denemefs_read_super(&superblocks[0]);

	init_task->root = init_task->pwd = superblocks[0].root;

	printf(">> mount_root OK\n");
}
