#ifndef CODE_EDITOR_H
#define CODE_EDITOR_H


struct Editor
{
    int cx, cy;  
    int rowoff;  
    int coloff; 
    int screenrows; 
    int screencols; 
};


int create_validate_file(char *filename);



#endif