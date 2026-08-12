#include <stdio.h>
#include <string.h>

int main(void)
{
    char cmdline[] = "motor set speed 1200"; // 원본 문자열 (수정 가능해야 함)
    char *cline;
    const char *delim = " ";
    
    char *argv[10] = {0};
    int argc = 0;
    char *next_ptr = NULL;
    char *tok = NULL;
    cline = &cmdline[0];

    for (tok = strtok_r(cline, delim, &next_ptr); tok; tok = strtok_r(NULL, delim, &next_ptr))
    {
        argv[argc++] = tok; //
    }
    //받은 문자열을 다시 분류해서 cli_t의 최외곽에 있는 cli_t->*argv[]에 하나씩 적립하는 형태같음.
    

    // 결과 출력
    printf("Total argc: %d\n", argc);
    for (int i = 0; i < argc; i++)
    {
        printf("argv[%d]: %s\n", i, argv[i]);
    }

    return 0;
}
