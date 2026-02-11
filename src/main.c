
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

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


#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL , 0}
#define KILO_VERSION "0.1"
#define KILO_TAB_STOP 8
#define KILO_QUIT_TIME 3



struct editor_config E;
void editor_set_message(const char *fmt , ...);
void editor_refresh_screen();
char *editor_prompt(char *prompt , void(*callback)(char * , int));




void editor_find_callback(char *query , int key){
    static int last_match = -1;
    static int direction = 1;
    if(key == '\r' || key == '\x1b'){
        last_match = -1;
        direction = 1;
        return;
    } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
        direction = 1;
    } else if (key == ARROW_LEFT || key == ARROW_UP) {
        direction = -1;
    } else {
        last_match = -1;
        direction = 1;
    }

    if( last_match == -1) direction = 1;
    int current = last_match;
    
    int i;
    for(int i = 0 ; i < E.numrows ; i++){
        current += direction;
        if(current == -1) current = E.numrows - 1;
        else if (current == E.numrows) current = 0;

        erow *row = &E.row[current];
        char *match = strstr(row->render , query);
        if (match){
            last_match = current;
            E.cy = current;
            E.cx = editor_row_rx_to_cx(row , match - row->render);
            E.rowoff = E.numrows;
            break;
        }
    }
}

void editor_find(){
    int saved_cx = E.cx;
    int saved_cy = E.cy;
    int saved_coloff = E.coloff;
    int saved_rowoff = E.rowoff;


    char *query = editor_prompt("Search: %s (Use ESC/Arrows/Enter)", editor_find_callback);
    if (query){
        free(query);
    } else {
        E.cx = saved_cx;
        E.cy = saved_cy;
        E.coloff = saved_coloff;
        E.rowoff = saved_rowoff;
    } 
    
}



/**
 *  
 */







/**
 * STDIN_FILENO 
 * It is the file descripter from standard input stdin
 * STDIN_FILENO == 0            // standard Input
 * STDOUT_FILENO == 1           // standard output
 * STDERR_FILENO == 2           //standard error
 * 
 */

int editor_read_key(){
    int nread;
    char c;
    while((nread = read(STDIN_FILENO , &c , 1)) != 1){
        if(nread == -1 && errno != EAGAIN) die("read");
    }
    if(c == '\x1b'){
        char seq[3];

        if(read(STDIN_FILENO , &seq[0] , 1) != 1) return '\x1b';
        if(read(STDIN_FILENO , &seq[1] , 1) != 1) return '\x1b';

        if(seq[0] == '['){
            if(seq[1] >= '0' && seq[1] <= '9'){
                if(read(STDIN_FILENO , &seq[2] , 1) != 1) return '\x1b';
                if(seq[2] == '~'){
                    switch(seq[1]){
                        case '1' : return HOME_KEY;
                        case '3' : return DEL_KEY;
                        case '4' : return END_KEY;
                        case '5' : return PAGE_UP;
                        case '6' : return PAGE_DOWN;
                        case '7' : return HOME_KEY;
                        case '8' : return END_KEY;
                    }
                }
            }else{
                switch(seq[1]){
                    case 'A' : return ARROW_UP;
                    case 'B' : return ARROW_DOWN;
                    case 'C' : return ARROW_RIGHT;
                    case 'D' : return ARROW_LEFT;
                    case 'H' : return HOME_KEY;
                    case 'F' : return END_KEY;
                }
            }  

        }else if (seq[0] == 'O'){
            switch (seq[1]) {
                case 'H' : return HOME_KEY;
                case 'F' : return END_KEY;
            }
        }
        return '\x1b';
    }else{
        return c;    
    }
}

char *editor_prompt(char *prompt, void(*callback)(char * , int)){
    size_t bufsize = 128;
    char *buf = malloc(bufsize);

    size_t buflen = 0;
    buf[0] = '\0';

    while(1){
        editor_set_message(prompt , buf);
        editor_refresh_screen();

        int c = editor_read_key();

        if(c = DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE){
            if(buflen != 0) buf[--buflen] = '\0';
        }else if(c == '\x1b'){
            editor_set_message("");
            if(callback) callback(buf , c);
            free(buf);
            return NULL;
        }else if(c == '\r'){
            if(buflen != 0){
                editor_set_message("");
                if(callback) callback(buf, c);
                return buf;
            }
        }else if(!iscntrl(c) && c < 128){
            if(buflen == bufsize - 1){
                bufsize *= 2;
                buf = realloc(buf , bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
        if(callback) callback(buf ,  c);
    }
}

void editor_move_curser(int key){
    erow  *row = ( E.cy >= E.numrows ) ? NULL : &E.row[E.cy];
    switch(key){
        case ARROW_LEFT:
            if(E.cx != 0){
                E.cx--;
            } else if(E.cy > 0){
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        case ARROW_RIGHT:
            if(row && E.cx < row->size){
                E.cx++;     
            } else if (row && E.cx == row->size){
                E.cy++;
                E.cx = 0;
            }
            break;   
        case ARROW_UP:
            if(E.cy != 0){
                E.cy--;
            }
            break;
        case ARROW_DOWN:
            if(E.cy < E.numrows){
                E.cy++;
            }
            break;
    }
    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if(E.cx > rowlen){
        E.cx = rowlen;
    }
}


void editor_process_keypress(){
    static int quit_times = KILO_QUIT_TIME;

    int c = editor_read_key();

    switch(c){
        case '\r':
            editor_insert_new_line();
            break;

        case CTRL_KEY('q'):
            if(E.dirty && quit_times > 0){
                editor_set_message("WARNING!!! File has unsaved changes. " "Press Ctrl-Q %d more times to quit." , quit_times);
                quit_times--;
                return;
            }

            write(STDOUT_FILENO , "\x1b[2J" , 4);
            write(STDOUT_FILENO , "\x1b[H" , 4);
            exit(0);
            break;

        case CTRL_KEY('s'):
            editor_save();
            break;

        case HOME_KEY:
            E.cx = 0;
            break;
        case END_KEY:
            if(E.cy < E.numrows){
                E.cx = E.screencols - 1;
            }
            break;

        case CTRL_KEY('f'):
            editor_find();
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if(c == DEL_KEY) editor_move_curser(ARROW_RIGHT);
            editor_del_char();
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                if (c == PAGE_UP) {
                    E.cy = E.rowoff;
                }else if (c == PAGE_DOWN){
                    E.cy = E.rowoff + E.screenrows - 1;
                    if (E.cy > E.numrows) E.cy = E.numrows;
                }

                int times = E.screenrows;
                while(times--){
                    editor_move_curser(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
                }
            }
            break;
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editor_move_curser(c);
            break;

        case CTRL_KEY('l'):
        case '\x1b':
            break;

        default : 
            editor_insert_char(c);
            break;
    }
    quit_times = KILO_QUIT_TIME;
}

void editor_draw_rows(struct abuf *ab){
    int y;
    for(y = 0 ; y < E.screenrows ; y++){
        int filerow = y + E.rowoff;
        if( filerow >= E.numrows){
            if(E.numrows == 0 && y == E.screenrows / 3){
                char welcome[80];
                int welcomelen = snprintf(welcome , sizeof(welcome) , "Rim Editor -- version %s" , KILO_VERSION);
                if(welcomelen > E.screencols) welcomelen = E.screencols;
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
        }else{
            int len = E.row[filerow].rsize - E.coloff;
            if(len < 0) len = 0;
            if (len > E.screencols) len = E.screencols;
            ab_append(ab , &E.row[filerow].render[E.coloff] , len);
        }

        ab_append(ab , "\x1b[K", 3);
        ab_append(ab , "\r\n" , 2);
    }
}

int editor_row_cx_to_cy(erow *row , int cx){
    int rx = 0;
    int j = 0;
    for(j = 0; j < cx; j++){
        if(row->chars[j] == '\t'){
            rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
        }
        rx++;
    }
    return rx;  
}

int editor_row_rx_to_cx(erow *row , int rx){
    int cur_rx = 0;
    int cx;
    for(cx = 0 ; cx < row->size ; cx++) {
        if(row->chars[cx] == '\t'){
            cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP);

        }
        cur_rx++;
        if (cur_rx > rx) return cx;
    }
    return cx;
}

void editor_scroll(){
    E.rx = 0;
    if(E.cy < E.numrows) {
        E.rx = editor_row_cx_to_cy(&E.row[E.cy] , E.cx);
    }

    if(E.cy < E.rowoff){
        E.rowoff = E.cy;
    }
    if(E.cy >= E.rowoff + E.screenrows){
        E.rowoff = E.cy - E.screenrows + 1;
    }
    if(E.rx < E.coloff){
        E.coloff = E.rx;
    }
    if(E.rx >= E.coloff + E.screencols){
        E.coloff = E.rx - E.screencols + 1;
    }
}

void editor_draw_status_bar(struct abuf *ab){
    ab_append(ab , "\x1b[7m" , 4);
    char status[80] , rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines" , E.filename ? E.filename : "[No Name]" , E.numrows , E.dirty ? "(modified)" : "");
    int rlen =  snprintf(rstatus , sizeof(rstatus) , "%d%d" , E.cy + 1 , E.numrows);

    if(len > E.screencols) len = E.screencols;
    ab_append(ab , status , len);
    while(len < E.screencols){
        if(E.screencols - len == rlen){
            ab_append(ab , rstatus , rlen);
            break;
        }else {
            ab_append(ab , " ", 1);
            len++;
        }     
    }
    ab_append(ab , "\x1b[m" , 3);
    ab_append(ab, "\r\n", 2);
}

void editor_draw_message_bar(struct abuf *ab) {
  ab_append(ab, "\x1b[K", 3);
  int msglen = strlen(E.statusmessage);
  if (msglen > E.screencols) msglen = E.screencols;
  if (msglen && time(NULL) - E.statusmes_time < 5)
    ab_append(ab, E.statusmessage, msglen);
}


void editor_refresh_screen(){
    editor_scroll();
    struct abuf ab = ABUF_INIT;

    ab_append(&ab , "\x1b[?25l" , 6);
    ab_append(&ab , "\x1b[H" , 3);

    editor_draw_rows(&ab);
    editor_draw_status_bar(&ab);
    editor_draw_message_bar(&ab);
    
    char buf[32];
    snprintf(buf , sizeof(buf) , "\x1b[%d;%dH" , (E.cy - E.rowoff) + 1 , (E.rx - E.coloff) + 1);
    ab_append(&ab , buf , strlen(buf));

    ab_append(&ab , "\x1b[H" , 3);
    ab_append(&ab , "\x1b[?25h" , 6);

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
    E.rx = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = NULL;
    E.dirty = 0;
    E.filename = NULL;
    E.statusmessage[0] = '\0';
    E.statusmes_time = 0;
    if(get_window_size(&E.screenrows , &E.screencols) == -1) die("getWindowSize");
    E.screenrows -= 2;
}

void editor_set_message(const char *fmt , ...){
    va_list ap;
    va_start(ap , fmt);
    vsnprintf(E.statusmessage , sizeof(E.statusmessage) , fmt , ap);
    va_end(ap);
    E.statusmes_time = time(NULL);

}

int main(int argc , char **argv){
    enable_raw_mode();
    init_editor();

    if(argc >= 2){
        editor_open(argv[1]);
    }

    editor_set_message("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

    while (1){
        editor_refresh_screen();
        editor_process_keypress();
    }
    return 0;
}
