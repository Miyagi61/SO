#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
typedef void (*sighandler_t)(int);

#define N_Filters 5

char *configs[100][4];
char* task_name[100];
int task;
int c_tasks[100];
char* filters;
int used[100];

int aplicaFiltros(char** args, int n_args){
    int p[2];  // p[0] = 3   p[1] = 4
    int idx,i;
    char fst[25][100];
    int stat;
    for(i = 3; i < (n_args-1) ; i++){
        for(idx=0; strcmp(args[i],configs[idx][0])!=0 ; idx++);
        sprintf(fst[i-3],"%s/%s",filters,configs[idx][1]);
    }
    for(i = 0; i < (n_args - 5); i++){
        pipe(p);
        pid_t filho = fork();
        if(!filho){
            dup2(p[1],1); close(p[1]); close(p[0]);    
            if(execl(fst[i],fst[i],NULL) == -1){
                perror("Erro a aplicar filtro"); _exit(-1);
            }
        }
        else{
            dup2(p[0],0); close(p[0]); close(p[1]);
            waitpid(filho,&stat,0);
            if(WEXITSTATUS(stat)==-1)
                return -1;
        }
    }

    if(execl(fst[i],fst[i],NULL) == -1){
        perror("Erro a aplicar filtro"); _exit(-1);
    }
    return 0;
}

int redir(char* args[]){
    int input,output;
    if( (input = open(args[1], O_RDONLY)) == -1){
        perror("open input"); return -1;
    }
    dup2(input,0); close(input);
    
    if( (output = open(args[2], O_TRUNC | O_WRONLY | O_CREAT, 0644)) == -1){
        perror("create output"); return -1;
    };
    dup2(output,1); close(output);
    return 0;
}

int checkFiltros(char *args[], int n_args){
    int i,idx;
    for(i = 3; i <= (n_args-2) ; i++){
        for(idx=0; idx < N_Filters && strcmp(args[i],configs[idx][0])!=0 ; idx++);
        if(idx == N_Filters)
            return -1;
    }
    return 0;
}

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
// t in out f1 f2 f3
void muda_Used(char* args[], int n_args, int s_d){
    int j;
    for(int i = 3; i < (n_args-1) ; i++){
        for(j = 0; strcmp(configs[j][0],args[i]) ; j++);
        used[j] = used[j] + 1*s_d;
    }
}


int main(int argc , char* argv[]){
    argv[1] = "etc/aurrasd.conf";   // RETIRAR ISTOOOOO!!!!!!!
    argv[2] = "bin/aurrasd-filters";
// ---------- sinais


// ---------- criacao do fifo e acerto de propriedades

    mkfifo("tmp/fifo",0622); // rw--w--w-
    mkfifo("tmp/status",0644); // rw-r--r--
    chmod("tmp/fifo",0622); // rw--w--w-
    chmod("tmp/status",0644); // rw-r--r--
    mkfifo("tmp/process",0644); // rw-r--r--
    chmod("tmp/process",0644); // rw-r--r--

// ----------- inicializador de ficheiros e verificação
    task=0;
    char *buf= malloc(1024);
    int config, fifo=-1, status, n,process;
    
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
        filters = strdup(argv[2]);
    
    for(int i = 0; i < 100 ; i++){
        used[i]=0;
        c_tasks[i]=-1;
    }

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
        task++;
        if(!fork()){
            while((n = read(fifo,buf,1024))){
                if(!strcmp(buf,"status")){
                    status = open("tmp/status",O_WRONLY,0644);
                    sendStatus(status);
                    close(status);
                }
                else{
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

                //------ redirecionamento ----------------

                    if(redir(exec_args)==-1){
                        return -1;
                    }
        
                //-----------------------------------------
                    process = open("tmp/process",O_WRONLY,0644);
                    write(process,"Pending\n",9);

                //-------------Paragem --------------------    

                    write(process,"Processing...\n",15);

                    if(checkFiltros(exec_args,word)==-1){
                        write(process,"Error\n",7);
                        close(process);
                        _exit(-1);
                    }

                    muda_Used(exec_args,word,1);

                    pid_t pid_filho = fork();
                    int ret;
                    if(!pid_filho){
                        _exit(aplicaFiltros(exec_args,word));
                    }
                    else{                       
                        waitpid(pid_filho,&ret,0);
                        muda_Used(exec_args,word,-1);
                        if(WEXITSTATUS(ret)==-1)
                            write(process,"Error\n",7);
                        else
                            write(process,"Done\n",6);
                        close(process);
                    }
                }
            }
            _exit(0);
        }
    }

    return 0;
}   
