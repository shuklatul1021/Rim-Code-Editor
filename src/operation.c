

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



char *editor_rows_to_string(int *buflen){
    int total_len = 0;
    int j;
    for(j = 0; j < E.numrows; j++){
        total_len += E.row[j].size + 1;
    }
    *buflen = total_len;
    char *buf = malloc(total_len);
    char *p = buf;
    for(j = 0 ; j < E.numrows ; j++){
        memcpy(p , E.row[j].chars , E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    return buf;
}


void editor_update_row(erow *row){
    int tabs = 0;
    int j;
    for(j = 0 ; j< row->size ; j++){
        if(row->chars[j] == '\t') tabs++;
    }
    
    free(row->render);
    row->render = malloc(row->size + tabs * (KILO_TAB_STOP -1 ) + 1);

    int idx = 0;
    for(j = 0 ; j< row->size; j++){
        if(row->chars[j] == '\t'){
            row->render[idx++];
            while( idx % KILO_TAB_STOP != 0 ) row->render[idx++] = ' ';
        }else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}


void editor_insert_row(int at , char *s , size_t len){
    if (at < 0 || at > E.numrows) return;

    E.row = realloc(E.row , sizeof(erow) * (E.numrows + 1));
    memmove(&E.row[at + 1] , &E.row[at] , sizeof(erow) * (E.numrows - at));

    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars , s , len);
    E.row[at].chars[len] = '\0';

    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    editor_update_row(&E.row[at]);

    E.numrows++;
    E.dirty++;
}

void editor_row_insert_char(erow *row , int at , int c){
    if(at < 0 || at > row->size) at = row->size;
    row->chars = realloc(row->chars , row->size + 2);
    memmove(&row->chars[at + 1] , &row->chars[at] , row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editor_update_row(row);
    E.dirty++;
}

void editor_row_append_string(erow *row , char *s , size_t len){
    row->chars = realloc(row->chars , row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editor_update_row(row);
    E.dirty++;
}


void editor_del_char(){
    if(E.cy == E.numrows ) return;
    if(E.cx == 0 && E.cy == 0) return;

    erow *row = &E.row[E.cy];
    if(E.cx > 0){
        editor_row_del_char(row , E.cx - 1);
        E.cx--;
    }else{
        E.cx = E.row[E.cy - 1].size;
        editor_row_append_string(&E.row[E.cy - 1] , row->chars , row->size);
        editor_del_row(E.cy);
        E.cy--;
    }
}

void editor_insert_new_line(){
    if(E.cx == 0){
        editor_insert_row(E.cy , "" , 0);
    }else {
        erow *row = &E.row[E.cy];
        editor_insert_row(E.cy + 1 , &row->chars[E.cx] , row->size - E.cx);
        row = &E.row[E.cy];
        row->size = E.cx;
        row->chars[row->size] = '\0';
        editor_update_row(row);
    }
    E.cy++;
    E.cx = 0;
}

void editor_insert_char(int c){
    if(E.cy == E.numrows){
        editor_insert_row(E.numrows , "" , 0);
    }
    editor_row_insert_char(&E.row[E.cy] , E.cx , c);
    E.cx++;
}


void editor_row_del_char(erow *row , int at){
    if (at < 0 || at >= row->size) return;
    memmove(&row->chars[at] , &row->chars[at + 1] , row->size - at);
    row->size--;
    editor_update_row(row);
    E.dirty++;
}

void editor_free_row(erow *row) {
    free(row->render);
    free(row->chars);
}

void editor_del_row(int at){
    if(at < 0 || at >= E.numrows) return;
    editor_free_row(&E.row[at]);
    memmove(&E.row[at] , &E.row[at + 1] , sizeof(erow) * (E.numrows - at - 1));
    E.numrows--;
    E.dirty++;
}