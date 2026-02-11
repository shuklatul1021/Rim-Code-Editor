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

void editor_open(char *filename ,  struct editor_config *E){
    free(E.filename);
    E.filename = strdup(filename);
    FILE *fp = fopen(filename , "r");
    if(!fp) die("open");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while((linelen = getline(&line , &linecap , fp)) != -1){
        while ( linelen > 0 && (line[linelen - 1] == '\n' || line[linelen -1] == '\r')) linelen--;
        editor_insert_row(E.numrows , line , linelen);
    }

    free(line);
    fclose(fp); 
    E.dirty = 0;
}


void ab_append(struct abuf *ab , const char *s , int len){
    char *new = realloc(ab->b , ab->len + len);

    if(new == NULL) return;
    memcpy(&new[ab->len] , s , len);
    ab->b = new;
    ab->len += len;
}

void ab_free(struct abuf *ab){
    free(ab->b);
}

void die(const char *s){
    write(STDOUT_FILENO , "\x1b[2J" , 4);
    write(STDOUT_FILENO , "\x1b[H" , 4);
    perror(s);
    exit(1);
}

void disable_row_mode(){
    if(tcsetattr(STDIN_FILENO , TCSAFLUSH , &E.orig_termios) == -1){
        die("tcsetattr");
    }
}


void enable_raw_mode(){
    if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disable_row_mode);

    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}