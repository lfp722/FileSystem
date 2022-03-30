/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>
#include <vnode.h>


/*
 * Put your function declarations and data types here ...
 */


struct f {
    struct vnode *fv;
    int flag;
    off_t offset;
};

//https://edstem.org/au/courses/7609/discussion/767658
//initialize open file table here
struct f OF_table[OPEN_MAX];

//functions
void OF_table_init(void);
int sys_open(const char *filename, int flags, mode_t mode);
int sys_close(int fd);
ssize_t sys_read(int fd, void *buf, size_t buflen);
ssize_t sys_write(int fd, const void *buf, size_t nbytes);
int sys_dup2(int oldfd, int newfd);
off_t sys_lseek(int fd, off_t pos, int whence);

#endif /* _FILE_H_ */
