#ifndef CODE_EDITOR_H
#define CODE_EDITOR_H

#include <termios.h>

typedef struct erow {
    int size;
    int rsize;
    char *chars;
    char *render;
} erow;

enum editor_key {
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,

    PAGE_UP,
    PAGE_DOWN,

    HOME_KEY,
    END_KEY,

    DEL_KEY
};

struct editor_config {
    int cx, cy; 
    int rx;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int numrows;
    erow *row;
    int dirty;
    char *filename;
    char statusmessage[80];
    time_t statusmes_time;
    struct termios orig_termios;
};

struct abuf {
    char *b;
    int len;
};


#endif