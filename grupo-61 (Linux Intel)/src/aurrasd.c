#include <sys/types.h>
#include <sys/stat.h>
   #include <fcntl.h>
    #include <unistd.h>
#include <string.h>
#include <stdio.h>

ssize_t readln(int fd, char *line, size_t size){
        int n,r=0,i=0;

        if((n=read(fd,line,size))>0){
                r+=n;
                for(i = 0; i < n && line[i] != '\n'; i++);
        }
        if(line[i]=='\n') i++;
        lseek(fd,i-r,SEEK_CUR);
return i;
}


int main(int argc , char* argv[]){

// ---------- criacao do fifo e acerto de propriedades

    mkfifo("fifo",0622);
    chmod("fifo",0622);

// ----------- inicializador de ficheiros e verificação

    char buf[1024]="";
    char* filters;
    int config, fifo, n;
    
    if( argv[1] ){
        if ((config = open(argv[1],O_RDONLY,0622)) == -1){
            perror("Erro a abrir ficheiro de configuração ");
            return -1;
        }
    }
    else{
        perror("Ficheiro de configuração inexistente");
        return -1;
    }

    if( !argv[2] ){
        perror("Pasta de filtros inexistente");
        return -1;
    }else 
        filters = argv[2];
    
    if ((fifo = open("fifo",O_RDONLY,0622)) == -1){
        perror("Erro a abrir pipe com nome");
        return -1;
    }

// --------------------------------------------
    printf("ola1");
    
    char *exec_args[100][10];
    int word = 1, lines = 0;
    exec_args[0][0] = buf;
    
    for(lines = 0; n = readln(config,buf,1024) ; lines++){
        for (int j = 0; buf[j]; j++){
            if (buf[j] == ' '){
                buf[j] = '\0';
                exec_args[word++][lines] = buf + j + 1;
            }
        }
        exec_args[word][lines] = NULL;
    }
    printf("ola");
    for(int i = 0; i < lines ; i++)
        for(int j = 0; j < word ; j++){
            printf("%s |",exec_args[j][i]);
        }
/*
    while(1){    
        while((n = read(fifo,buf,1024))){      
            
        }
        fifo = open("fifo",O_RDONLY,0622);
    } */ 
    return 0;
}   
