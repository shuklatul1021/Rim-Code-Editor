
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "editor.h"


void editor_save(struct editor_config *E){
    if(E->filename == NULL){
        E->filename = editor_prompt("Save As: %s(ESC to cancel)" , NULL);
        if(E->filename == NULL){
            editor_set_message("Save Aborted");
            return;
        }
    }

    int len;
    char *buf = editor_rows_to_string(&len);
    int fd = open(E->filename , O_RDWR | O_CREAT , 0644);
    if(fd != -1){
        if((ftruncate(fd , len)) != -1){
            if((write(fd , buf , len)) == len){
                close(fd);
                free(buf);
                E.dirty = 0;
                editor_set_message("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }
    
    free(buf);
    editor_set_message("Can't save! I/O error: %s", strerror(errno));
}