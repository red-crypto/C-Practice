#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
 
int cliPrintf(const char *fmt, ...)
{   
    char buf[256];
    va_list args;
    int len =0;

    va_start(args,fmt);

    len = vsnprintf(buf,256,fmt,args);
    printf("strings : %s \r\n", buf);
    va_end(args);


    return 0;
}

int main(void)
{
    cliPrintf("안녕하세요\r\n");

    return 0;
}