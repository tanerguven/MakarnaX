#ifndef _TASK_FS_H
#define _TASK_FS_H

#define TASK_MAX_FILE_NR 32

struct Task_FS {
	struct dirent * pwd;
	struct dirent * root;
	/* struct inode * executable; */
	struct file *files[TASK_MAX_FILE_NR];
};

extern int fs_fork(Task *c);
extern int fs_task_init(Task *t, struct dirent *root);
extern void task_curr_free_files();

#endif /* _TASK_FS_H */
