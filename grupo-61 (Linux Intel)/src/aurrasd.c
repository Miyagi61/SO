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


char *configs[100][4];
int task;
int n_filters;
char* current_requests[100];
char* filters;
int used[100];
int requests_pid[100];
char* pending_requests[100];
int n_pending;

char* pointerToString(int argc, char** argv){
    char*buf = malloc(1024);
    int size = 1024;
    strcpy(buf,"");
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
            };
        }   
    }
    return buf;
}

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

void addPendingRequest(char *args[], int n_args){
    char * buf = malloc(1024);
    strcpy(buf,args[0]);
    strcat(buf," ");
    pending_requests[n_pending++] = strcat(buf,pointerToString(n_args,args));
}

int checkFiltros(char *args[], int n_args){
    int i,idx;
    for(i = 4; i <= (n_args-1) ; i++){
        for(idx=0; idx < n_filters && strcmp(args[i],configs[idx][0])!=0 ; idx++);
        if(idx == n_filters){
            kill(atoi(args[0]),SIGALRM);
            return 0;
        }else{
            if(used[idx] == atoi(configs[idx][2])){
                addPendingRequest(args,n_args);
                return 0;
            }
        }
            
    }
    return 1;
}

void sendStatus(int fifo){
    char* buf = malloc(1024);
    char* buf_tasks = malloc(1024);
    for(int i = 0; i < 100 ; i++){
        if(current_requests[i] != NULL){
            sprintf(buf_tasks,"task #%d: %s\n",i,current_requests[i]);
            write(fifo,buf_tasks,strlen(buf_tasks));
        }
    }
    for(int i = 0; i < n_filters ; i++){
        write(fifo,"filter ",8);
        write(fifo,configs[i][0],strlen(configs[i][0]));        
        sprintf(buf,": %d/%s (running/max)\n",used[i],configs[i][2]);
        write(fifo,buf,strlen(buf));
    }
    sprintf(buf,"pid: %d\n",getpid());
    write(fifo,buf,strlen(buf));
}

void muda_Used(char* args[], int n_args, int s_d){
    int j;
    int count[n_filters];

    for(int i = 0; i < n_filters ; i++)
        count[i] = 0;

    for(int i = 3; i < (n_args-1) ; i++){
        for(j = 0; strcmp(configs[j][0],args[i]) ; j++);
        count[j]++;
    }

    for(j = 0; j < n_filters ; j++){
        if(count[j]){
            used[j] = used[j] + 1*s_d;
        }   
    }
}


void clean_current_request(char* buf){
    int i;
    for(i = 0; i < 100; i++){
        if(current_requests[i] && !strcmp(buf,current_requests[i])){
            current_requests[i] = NULL;
            break;
        }
    }  
}

int* testPending(int *number_of_pending){ 
    int idx;
    int check = 1;
    char* buf;
    char *args[100];
    int n_args;
    int* pending_idx = malloc(sizeof(int)*100);

    for(int i = 0; i < n_pending ; i++){
        n_args = 0;
        buf = strdup(pending_requests[i]);
        for (int i = 0; buf[i]; i++){
            if (buf[i] == ' '){
                buf[i] = '\0';
                args[n_args++] = buf + i + 1;
                }
            else if( buf[i] == '\n'){
                buf[i] = '\0';
            }
        }
        args[n_args] = NULL;

        for(int j = 3; j < n_args ; j++){
            for(idx=0; idx < n_filters && strcmp(args[j],configs[idx][0])!=0 ; idx++);
            if(used[idx] == atoi(configs[idx][2])){
                check = 0;
            }
        }
        if(check == 1){
            pending_idx[(*number_of_pending)++] = i;
        }
        else
            check = 1;
        free(buf);      
    }
    if(*number_of_pending == 0) 
        return NULL;
    else{
        pending_idx[*number_of_pending] = -1;
        return pending_idx;
    }
        
}

void removePending(int* idx, int n){
    for(int i = 0; i < n; i++){
        free(pending_requests[idx[i]]);
        for(int j = idx[i]; j <= n_pending ; j++){
        pending_requests[j] = pending_requests[j+1]; 
        }
        n_pending--;
    }
    
}

char* pending_buffer(char* buf, int* idx , int n){
    char* new_buf = malloc(2048);
    strcpy(new_buf,buf);
    for(int i = 0; i < n ; i++){
        strcat(new_buf,pending_requests[idx[i]]);
    }
    return new_buf;
}

void handler(int s){
    int *pending_switch;
    char* buf = malloc(2048);
    int ready_pending,stat;
    char* main_buf = malloc(1024);
    int number=0;
    if(s == SIGTERM){
        printf("\nWaiting for current processes...\n");
        for(int i = 0; i < 100 ; i++){
            if(current_requests[i] != NULL){
                number++;
                if(!fork()){
                    wait(&stat);
                    kill(requests_pid[i],SIGKILL);
                    _exit(0);
                }       
            }
        }
        for(int i = 0; i < number; i++){
            wait(&stat);
        }
        printf("\nStarting pending processes...\n");
        for(int i = 0; i < 100 ; i++){
            used[i] = 0;
        }
        if(n_pending > 0){
            ready_pending = 0;
            if((pending_switch = testPending(&ready_pending))!=NULL ){
                buf = pending_buffer(buf,pending_switch,ready_pending);
                removePending(pending_switch,ready_pending);  
            }    
        }else
            buf = NULL;
        
        number = 0;
        while(buf && strcmp(buf,"")){
            number++;
            main_buf= strsep(&buf,"\n");                  
            char *exec_args[100];
            int word = 1;
            exec_args[0] = main_buf;
            for (int i = 0; main_buf[i]; i++){
                if (main_buf[i] == ' '){
                    main_buf[i] = '\0';
                    exec_args[word++] = main_buf + i + 1;
                }
            }
            exec_args[word] = NULL;          
            //-----------------------------------------
            if (kill(atoi(exec_args[0]),SIGUSR1)==-1){
                printf("Cliente incontactável\n");
            }
            else{
            //-------------Paragem --------------------        
                if(!fork()){
                //------ redirecionamento ----------------
                    if( redir(&exec_args[1]) == -1){
                        kill(atoi(exec_args[0]),SIGALRM);
                        exit(-1);
                    }

                    pid_t pid_filho = fork();
                    int ret;
                    if(!pid_filho){
                        _exit(aplicaFiltros(&exec_args[1],word));
                    }
                    else{                       
                        waitpid(pid_filho,&ret,0);
                        if(WEXITSTATUS(ret)==-1)
                            kill(atoi(exec_args[0]),SIGINT);
                        else
                            kill(atoi(exec_args[0]),SIGUSR2);
                    }
                    _exit(0);
                }
            }
        }
        for(int i = 0; i < number ; i++){
            wait(&stat);
        }
        printf("\nTerminando de forma graciosa\n");
        kill(getpid(),SIGKILL);
    }
}

int main(int argc , char* argv[]){
    argv[1] = "etc/aurrasd.conf";
    argv[2] = "bin/aurrasd-filters/";
// ---------- sinais

    signal(SIGTERM,handler);

// ---------- criacao do fifo e acerto de propriedades

    mkfifo("tmp/fifo",0622); // rw--w--w-
    mkfifo("tmp/status",0644); // rw-r--r--
    chmod("tmp/fifo",0622); // rw--w--w-
    chmod("tmp/status",0644); // rw-r--r--

// ----------- inicializador de ficheiros e verificação
    task=0;
    char *buf= malloc(1024);
    int config, fifo=-1, status, n;
    n_pending = 0;
    int *pending_switch = NULL;

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
        current_requests[i] = NULL;
        pending_requests[i] = NULL;
    }

// ------------ inicializador do array de configuração
    
    read(config,buf,1024);
    n_filters=0;
    int enter_aux;
    for(enter_aux = 0; buf[enter_aux] ; enter_aux++){
        if(buf[enter_aux]=='\n')
            n_filters++;
    }
    if(buf[enter_aux-1] != '\n')
        n_filters++;

    for(int j = 0; j < n_filters ; j++){
        for(int i = 0; i < 3 ; i++)
            configs[j][i] = strdup(strsep(&buf," \n"));

        configs[j][3] = NULL;
    }

    buf=malloc(1024);
    char* main_buf = malloc(1024);
    int ready_pending;
//  ---------Abertura do FIFO
    while(1){        
    
        if ((fifo = open("tmp/fifo",O_RDONLY,0622)) == -1){
            perror("Erro a abrir FIFO");
             return -1;
        }
        while((n = read(fifo,buf,1024)) > 0){
            while(buf && strcmp(buf,"")){
            main_buf= strsep(&buf,"\n"); 
            char *exec_args[100];
            int word = 1;
            exec_args[0] = main_buf;
            for (int i = 0; main_buf[i]; i++){
                if (main_buf[i] == ' '){
                    main_buf[i] = '\0';
                    exec_args[word++] = main_buf + i + 1;
                }
            }
            exec_args[word] = NULL;

            if(!strcmp(main_buf,"status")){
                status = open("tmp/status",O_WRONLY,0644);
                sendStatus(status);
                close(status);
            }
            else if(!strcmp(exec_args[0],"used")){
                muda_Used(&exec_args[2],word-1,-1);
                char *elim = pointerToString(word-1,&exec_args[1]);
                clean_current_request(elim);
                free(elim);

                if(n_pending > 0){
                ready_pending = 0;

                    if((pending_switch = testPending(&ready_pending))!=NULL ){
                        buf = pending_buffer(buf,pending_switch,ready_pending);
                        removePending(pending_switch,ready_pending);
                    }    
                }
            }            
            else if(checkFiltros(exec_args,word)){           
            //-----------------------------------------

                if (kill(atoi(exec_args[0]),SIGUSR1)==-1){
                    printf("Cliente incontactável\n");
                }
                else{
                    requests_pid[task] = atoi(exec_args[0]);
                    current_requests[task++] = pointerToString(word,exec_args);
            //-------------Paragem --------------------    
                
                    muda_Used(&exec_args[1],word,1);
                    if(!fork()){
                //------ redirecionamento ----------------
                        if( redir(&exec_args[1]) == -1){
                            kill(atoi(exec_args[0]),SIGALRM);
                            exit(-1);
                        }

                        pid_t pid_filho = fork();
                        int ret;
                        if(!pid_filho){
                            _exit(aplicaFiltros(&exec_args[1],word));
                        }
                        else{                       
                            waitpid(pid_filho,&ret,0);
                            if(WEXITSTATUS(ret)==-1)
                                kill(atoi(exec_args[0]),SIGINT);
                            else
                                kill(atoi(exec_args[0]),SIGUSR2);
                        }
                        _exit(0);
                    }
                }
            }    
        }
        }
    }

    return 0;
}   
