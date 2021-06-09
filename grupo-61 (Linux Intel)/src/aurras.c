#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


char* pointerToString(int argc, char** argv){
    char*buf = malloc(1024);
    int size = 1024;
    for(int i = 1; i < argc ; i++){
        strcat(buf,argv[i]);
        strcat(buf," ");
        if(strlen(buf) > size*(2/3)){
            size*=2;
            if(realloc(buf,size)==NULL){
                perror("Erro a realocar memória");
                return NULL;
            };
        }   
    }
    return buf;
}

int main(int argc, char** argv){
    int file;
    char buf[1024];
    int n=0;
    
    // --------- info

    if(argc == 1){
        printf("./aurras status\n");
        printf("./aurras transform input-filename output-filename filter-id-1 filter-id-2 ...\n");
    }
    // ---------- status

    else if( argc == 2 && !strcmp(argv[1],"status")){
        if( (file = open("tmp/fifo",O_WRONLY)) == -1){
            perror("Erro a abrir FIFO");
            return -1;
        }

        write(file,argv[1], 7);
        
        if( (file = open("tmp/status",O_RDONLY)) == -1){
            perror("Erro a abrir FIFO_STATUS");
            return -1;
        }
        
        while((n = read(file,buf,1024))){      
            write(STDOUT_FILENO,buf,n);
        }
    }
    // ---------- transform

    else if( argc > 4 && !strcmp(argv[1],"transform")){
    
        if( (file = open("tmp/fifo",O_WRONLY)) == 1){
            perror("Erro a abrir FIFO");
            return -1;
        }
        
        char* str;
        if((str = pointerToString(argc,argv))==NULL){
            return -1;
        }
        
        write(file,str,strlen(str));
        free(str);
        close(file);
        printf("pending\n");
        //------------verificacao

        //-----------------------
        printf("processing\n");
        //------------verificacao

        //-----------------------
        printf("done\n");
    }
    
    else{
        printf("Input inválido\n");
    }

    return 0;

}  
   
