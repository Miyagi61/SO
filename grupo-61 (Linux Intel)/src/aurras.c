#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char** argv){
    int i;
    char buf[1024];
    int n=0,j=0;
    char teste[1024];
    i = open("p_2_fifo",O_WRONLY,0622);
       
    for(; *(++argv) ;)
    {
        write(i,*argv,strlen(*argv));
    }

    i = open("p_2_fifo",O_RDONLY,0622);
}  
   
