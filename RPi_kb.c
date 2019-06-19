#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>    

void clear_report(char *report)
{
int i;
    for (i = 0; i < 10; i++) report[i] = 0;
}
void main()
{
char bytes[10];

    int fd = open("/dev/hidg0", O_RDWR);
    clear_report(&bytes[0]); 
    bytes[2] = 4;
    write(fd,&bytes[0],8);
    clear_report(&bytes[0]); 
    write(fd,&bytes[0],8);
    close (fd);
}
