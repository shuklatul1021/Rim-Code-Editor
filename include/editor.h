#ifndef CODE_EDITOR_H
#define CODE_EDITOR_H
#include <termios.h>

typedef struct erow {
    int size;
    char *chars;
} erow;

enum editor_key {
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
    int screenrows;
    int screencols;
    int numrows;
    erow *row;
    struct termios orig_termios;
};

struct abuf {
    char *b;
    int len;
};





#endif