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


char* str;
pid_t pid;

void handler(int s){
    if(s == SIGUSR1){
        printf("Processing...\n");
    }
    else if( s == SIGUSR2 || s == SIGINT){
        int file;
        char str_aux[1024];
        if(s == SIGUSR2)
            printf("Done\n");
        else
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
    else if( s == SIGALRM ){
        printf("Filtros inválidos\n");
        kill(pid,SIGKILL);
    }
}

int main(int argc, char** argv){
    int file;
    char buf[1024];
    int n=0;
    pid = getpid();

    // --------- sinais
    signal(SIGUSR1,handler);
    signal(SIGUSR2,handler);
    signal(SIGINT,handler);
    signal(SIGALRM,handler);
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

    // ------- P|P|D/E ----------- 
     /*   if( (file = open("tmp/process",O_RDONLY)) == -1){
            perror("Erro a abrir FIFO_PROCESS");
            return -1;
        }
    
        while(n = read(file,buf,1024))    
            write(STDOUT_FILENO,buf,n);
        close(file);
      
      // ------- diminuir used -----
        if( (file = open("tmp/fifo",O_WRONLY)) == -1){
            perror("Erro a abrir FIFO");
            return -1;
        }
        argv[1]="used";
        char* str2;
        if((str2 = pointerToString(argc,argv))==NULL){
            return -1;
        }        
        write(file,str2,strlen(str));
        free(str2);
        close(file);
        return 0;*/
        pause();
        pause();
    }
    else{
        printf("Input inválido\n");
    }

    return 0;

}  
   
