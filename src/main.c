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
#include <string.h>
#include <errno.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL , 0}
#define KILO_VERSION "0.1"

struct editor_config {
    int cx, cy;   //curser axis 
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

struct abuf {
    char *b;
    int len;
}
;
struct editor_config E;


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

/**
 * STDIN_FILENO 
 * It is the file descripter from standard input stdin
 * STDIN_FILENO == 0            // standard Input
 * STDOUT_FILENO == 1           // standard output
 * STDERR_FILENO == 2           //standard error
 * 
 */

char editor_read_key(){
    int nread;
    char c;
    while((nread = read(STDIN_FILENO , &c , 1)) != 1){
        if(nread == -1 && errno != EAGAIN) die("read");
    }
    if(c == '\x1b'){
        char seq[3];
    }else{
        return c;    
    }
}

void editor_move_curser(char key){
    switch(key){
        case 'a':
            E.cx--;
            break;
        case 'd':
            E.cx++;
            break;
        case 'w':
            E.cx--;
            break;
        case 's':
            E.cx++;
            break;
    }
}

void editor_process_keypress(){
    char c = editor_read_key();

    switch(c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO , "\x1b[2J" , 4);
            write(STDOUT_FILENO , "\x1b[H" , 4);
            exit(0);
            break;
        case 'w':
        case 's':
        case 'a':
        case 'd':
            editor_move_curser(c);
            break;
    }
}

void editor_draw_rows(struct abuf *ab){
    int y;
    for(y = 0 ; y < E.screenrows ; y++){
        if(y == E.screenrows / 3){
            char welcome[80];
            int welcomelen = snprintf(welcome , sizeof(welcome) , "Rim Editor -- version %s" , KILO_VERSION);
            if(welcomelen > E.screencols) welcomelen = E.screencols;
            //Center the Welcome Screen
            int padding = (E.screencols - welcomelen) / 2;
            if(padding){
                ab_append(ab , "~" , 1);
                padding--;
            }
            while(padding --) ab_append(ab , " ", 1);
            ab_append(ab , welcome , welcomelen);
        }else{
            ab_append(ab , "~" , 1);
        }

        

        ab_append(ab , "\x1b[K", 3);
        if(y < E.screenrows - 1 ){
            ab_append(ab , "\r\n" , 2);
        }
    }
}



void editor_refresh_screen(){
    struct abuf ab = ABUF_INIT;

    ab_append(&ab , "\x1b[?25J" , 6);
    ab_append(&ab , "\x1b[H" , 3);

    editor_draw_rows(&ab);
    
    char buf[32];
    snprintf(buf , sizeof(buf) , "\x1b[%d;%dH" , E.cy + 1 , E.cx + 1);
    ab_append(&ab , buf , strlen(buf));

    ab_append(&ab , "\x1b[H" , 3);
    ab_append(&ab , "\x1b[?25J" , 6);

    write(STDOUT_FILENO , ab.b , ab.len);
    ab_free(&ab);
}


int get_curser_postion(int *row , int *col){
    char buf[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO , "\x1b[6n", 4) != 4) return -1;

    while (i < sizeof(buf) - 1)
    {
        if(read(STDIN_FILENO , &buf[i] , 1) != 1) break;
        if(buf[i] == 'R') break;
        i++;
    }
    buf[i] = "\0";
    if(buf[0] != '\x1b' || buf[1] != '[') return -1;
    if(sscanf(&buf[2] , "%d;%d" , row , col) != 2) return -1;

    return 0;  
}

int get_window_size(int *rows , int *cols){
    struct winsize ws;
    if(ioctl(STDOUT_FILENO , TIOCGWINSZ , &ws) == -1 || ws.ws_col == 0){
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return get_curser_postion(rows , cols);
    }else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void init_editor(){
    E.cx = 0;
    E.cy = 0;
    if(get_window_size(&E.screenrows , &E.screencols) == -1) die("getWindowSize");
}

int main(){
    enable_raw_mode();
    init_editor();

    while (1){
        editor_refresh_screen();
        editor_process_keypress();
    }
    return 0;
}
