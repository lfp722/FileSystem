#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <file.h>
#include <syscall.h>
#include <copyinout.h>

/*
 * Add your file-related functions here ...
 */

void OF_table_init(void){
    for(int i=0;i<OPEN_MAX;i++){
        OF_table[i].fv = 0;
        OF_table[i].flag = 0;
        OF_table[i].offset = OPEN_MAX;
    }
}

int sys_open(const char *filename, int flags, mode_t mode){

    //check if filename is not null
    if(filename == NULL){
        return ENOMEM;
    }
    
    char *path = kmalloc(NAME_MAX*sizeof(char));

    //https://edstem.org/au/courses/7609/discussion/766836
    int a = copyinstr((userptr_t)filename, path, PATH_MAX, NULL);

    //if tehre is error in copyinstr
    if(a){
        return a;
    }

    //initialize vnode
    struct vnode *v;

    //open a file here using vfs_open
    int opened_file= vfs_open((char*)filename, flags, mode, &v);

    //if there is error in vfs_open
    if (opened_file){
        return opened_file;
    }
    
    //As we opened a file, we need to find an entry to the open file table
    //remember position for open file table
    int not_found = 1;
    int i;
    for(i = 0;i<OPEN_MAX;i++){
        //if there is free space in open file table,
        if(OF_table[i].fv == NULL){
            OF_table[i].fv = v;
            OF_table[i].flag = flags;
            OF_table[i].offset = OF_table[i].offset - 1;
            not_found = 0;
            break;
        }
    }

    //if there was no free space in open file table, return error
    if (not_found != 0){
        return ENFILE;
    }

    curproc -> FD_table[i] = i;
    return 0;
}


int sys_close(int fd){
    //close it
    vfs_close(OF_table[fd].fv);
    
    //clear allocated Open file table 
    OF_table[fd].flag = 0;
    OF_table[fd].offset += 1;

    //set the file descriptor table at index fd, unused
    if(curproc -> FD_table[fd]){
        curproc -> FD_table[fd] = -1;
    }
    return 0;
}


ssize_t sys_read(int fd, void *buf, size_t buflen){
    //if given fd is not valid, return error
    if(curproc -> FD_table[fd] == -1){
        return EBADF;
    }

    struct iovec iovec;
    struct uio uio;
    uio_uinit(&iovec, &uio, buf, buflen, OF_table[fd].offset, UIO_READ);
    int a = VOP_READ(OF_table[fd].fv, &uio);
    //if there was error during VOP_READ
    if(a){
        return a;
    }

    return 0;
}


ssize_t sys_write(int fd, const void *buf, size_t nbytes){
    //if given fd is not valid, return error
    if(curproc -> FD_table[fd] == -1){
        return EBADF;
    }

    struct iovec iovec;
    struct uio uio;
    uio_uinit(&iovec, &uio, (void*)buf, nbytes, OF_table[fd].offset, UIO_WRITE);
    int a = VOP_WRITE(OF_table[fd].fv, &uio);

    //if there was error during VOP_WRITE
    if(a){
        return a;
    }

    return 0;
}


int sys_dup2(int oldfd, int newfd){
    //if there is no allocated file in oldfd, return error
    if (curproc -> FD_table[oldfd] == -1){
        return EBADF;
    }
    curproc -> FD_table[newfd] = curproc -> FD_table[oldfd];
    curproc -> FD_table[oldfd] = -1;
    return 0;
}


off_t sys_lseek(int fd, off_t pos, int whence){
    if (curproc -> FD_table[fd] == -1){
        return EBADF;
    }
    if(whence == SEEK_SET){
        OF_table[fd].offset = pos;
    }
    else if (whence == SEEK_CUR){
        OF_table[fd].offset += pos;
    }
    else if (whence == SEEK_END){
        struct stat a;
        int ret = VOP_STAT(OF_table[fd].fv, &a);
        if (ret) {
            return ret;
        }
        OF_table[fd].offset += pos;
    }
    else {
        return EINVAL;
    }
    return 0;
}