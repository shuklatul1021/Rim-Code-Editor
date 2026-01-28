  int user_file_name;
    bool is_help = false;
    int fd;


    char c;

    if(argc <= 1){
        printf("Argument Requrire To use This:\n\t Use -h to get help");
    }

    while((user_file_name = getopt(argc , argv , ":f:h:")) != -1){
        switch(user_file_name){
            case 'f':
                filename = optarg;
                break;
            case 'h':
                is_help = true;
                break;
        }
    }

    if(is_help){
        help();
    }

    if(!filename){
        printf("Please Provide The File Name You want To edit");
    }

    fd = create_validate_file(filename);
    if(fd == -1){
        printf("The Create Validtor File Is Fail Try Again");
        return -1;
    }