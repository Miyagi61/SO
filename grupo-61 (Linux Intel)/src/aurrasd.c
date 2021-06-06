#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define N_Filters 5

char *configs[N_Filters][4];
int used[N_Filters];
int task;

void sendStatus(int fifo){
    char* buf = malloc(1024);
    for(int i = 0; i < N_Filters ; i++){
        write(fifo,"filter ",8);
        write(fifo,configs[i][0],strlen(configs[i][0]));        
        sprintf(buf,": %d/%s (running/max)\n",used[i],configs[i][2]);
        write(fifo,buf,strlen(buf));
    }
    sprintf(buf,"pid: %d\n",getpid());
    write(fifo,buf,strlen(buf));
}

int aplicaFiltros(char** args, int n_args){
    int p[2];  // p[0] = 3   p[1] = 4
    int idx,i;
    char* cmd[25][3];
    for(i = 3; i <= (n_args-2) ; i++){
        for(idx=0; strcmp(args[i],configs[idx][0])!=0 && idx < N_Filters ; idx++);
        if(idx == N_Filters) 
            return -1;
        else{
            cmd[i-3][0]=strdup(configs[idx][1]);
            cmd[i-3][1]=cmd[i-3][0];
            cmd[i-3][2]=NULL;
        }
    }/*
    for(i = 0; i < (n_args - 4); i++){
        pipe(p); // sempre antes do fork
        if(!fork()){
            dup2(p[1],1); close(p[1]); close(p[0]);;
            execvp(cmd[i][0],cmd[i]);
            _exit(1);        
        }
        else{
            dup2(p[0],0); close(p[0]); close(p[1]);
        }
    }
    execvp(cmd[i][0],cmd[i]);*/
    return 0;
}


int main(int argc , char* argv[]){
    argv[1] = "etc/aurrasd.conf";
    argv[2] = "bin/aurras-filters";

// ---------- criacao do fifo e acerto de propriedades

    mkfifo("tmp/fifo",0622); // rw--w--w-
    mkfifo("tmp/status",0644); // rw-r--r--
    chmod("tmp/fifo",0622); // rw--w--w-
    chmod("tmp/status",0644); // rw-r--r--

// ----------- inicializador de ficheiros e verificação
    task=0;
    char *buf= malloc(1024);
    char* filters;
    int config, fifo=-1, status, n;
    
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
    

// ------------ inicializador do array de configuração
    
    read(config,buf,1024);

    for(int j = 0; j < N_Filters ; j++){
        for(int i = 0; i < 3 ; i++)
            configs[j][i] = strdup(strsep(&buf," \n"));

        configs[j][3] = NULL;
    }
    free(buf);
    buf=malloc(1024);
    
//  ---------Abertura do FIFO
    while(1){
        if ((fifo = open("tmp/fifo",O_RDONLY,0622)) == -1){
            perror("Erro a abrir FIFO");
            return -1;
        }
        while((n = read(fifo,buf,1024))){
            if(!strcmp(buf,"status")){
                status = open("tmp/status",O_WRONLY,0644);
                sendStatus(status);
                close(status);
            }else{
                // ---- divisao do buf em palavras ----
                char *exec_args[100];
                int word = 1;
                exec_args[0] = buf;
                for (int i = 0; buf[i]; i++){
                    if (buf[i] == ' '){
                        buf[i] = '\0';
                        exec_args[word++] = buf + i + 1;
                    }
                }
                exec_args[word] = NULL;

                if(aplicaFiltros(exec_args,word)==-1){
                    printf("Filtro inválido\n");
                }
            }
        }
    } 

    return 0;
}   
