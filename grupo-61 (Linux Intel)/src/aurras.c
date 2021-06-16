#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
typedef void (*sighandler_t)(int);

char* pointerToString(int argc, char** argv){
    char*buf = malloc(1024);
    int size = 1024;
    strcpy(buf,"");
    pid_t pid = getpid();
    sprintf(buf,"%d ",pid); 
    for(int i = 1; i < argc ; i++){
        strcat(buf,argv[i]);
        if(i == argc - 1)
            strcat(buf,"\n");
        else
            strcat(buf," ");
        if(strlen(buf) > size*(2.0/3)){
            size*=2;
            if(realloc(buf,size)==NULL){
                perror("Erro a realocar memória");
                return NULL;
            }
        }   
    }
    return buf;
}

int working;
char* str;
pid_t pid;

void handler(int s){
    if(s == SIGUSR1){
        printf("Processing...\n");
        working = 1;
    }
    else if( s == SIGUSR2 || s == SIGINT){
        int file;
        char str_aux[1024];
        if(s == SIGINT && working == 0){
            printf("\nInterrupted while pending\n");
            kill(pid,SIGKILL);
        }
        else{
            if(s == SIGUSR2)
                printf("Done\n");
            else if( s == SIGINT && working == 1)
                printf("Error\n");

            if( (file = open("tmp/fifo",O_WRONLY)) == -1){
                perror("Erro a abrir FIFO");
            }
            strcpy(str_aux,"");
            strcat(str_aux,"used ");
            strcat(str_aux,str);
        
            write(file,str_aux,strlen(str_aux));
            free(str);
            close(file);
            kill(pid,SIGKILL);
        }
    }
    else if( s == SIGALRM ){
        printf("Input inválido ou Servidor fechou\n");
        kill(pid,SIGKILL);
    }
}

int main(int argc, char** argv){
    int file;
    char buf[1024];
    int n=0;
    pid = getpid();
    working = 0;

    // --------- sinais ----------------------------------------

    signal(SIGUSR1,handler);
    signal(SIGUSR2,handler);
    signal(SIGINT,handler);
    signal(SIGALRM,handler);

    // ---------- info -----------------------------------------

    if(argc == 1){
        printf("./aurras status\n");
        printf("./aurras transform input-filename output-filename filter-id-1 filter-id-2 ...\n");
    }
    // ---------- status ----------------------------------------

    else if( argc == 2 && !strcmp(argv[1],"status")){
        if( (file = open("tmp/fifo",O_WRONLY)) == -1){
            perror("Erro a abrir FIFO");
            return -1;
        }

        char* aux = malloc(1024);
        sprintf(aux,"%s\n",argv[1]);
        write(file,aux, strlen(aux));
        free(aux);

        if( (file = open("tmp/status",O_RDONLY)) == -1){
            perror("Erro a abrir FIFO_STATUS");
            return -1;
        }
        
        while((n = read(file,buf,1024))){      
            write(STDOUT_FILENO,buf,n);
        }
    }
    // ---------- transform ---------------------------------------

    else if( argc > 4 && !strcmp(argv[1],"transform")){
        printf("Pending\n");
        
        if( (file = open("tmp/fifo",O_WRONLY)) == -1){
            perror("Erro a abrir FIFO");
            return -1;
        }
        
        if((str = pointerToString(argc,argv))==NULL){
            return -1;
        }
        
        write(file,str,strlen(str));
        close(file);

        pause();
        pause();
    }
    else{
        printf("Input inválido\n");
    }

    return 0;

}  
   
