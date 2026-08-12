#include <stdio.h>

typedef struct
{
    int argc;
    char **argv;
} cli_args_t;


int cliParseArgs(char *line,
                 char *argv_buf[],
                 int argv_max,
                 cli_args_t *out)
{
    // TODO
    argv_buf[0] = &line[0];
    argv_buf[1] = &line[6];
    argv_buf[2] = &line[10];
    //그리고 각 끝나는 부분마다 \0를 넣어줘야함.
    line[5] = '\0';
    line[9] = '\0';
    line[13] = '\0'; // 이건 필수는 아닌데 규칙성을 위해서
    out->argv = argv_buf; //주소값 매칭
    out->argc = 3;

    //=> 위 항목을 이제 일반화 시키면된다.
}

int main(void)
{
    char line[] = "motor 100 200";
    //line index 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
    //     value m o t o r   1 0 0   2  0  0  \0

    char *argv[8];
    cli_args_t args;

    cliParseArgs(line, argv, 8, &args);

    printf("argc = %d\n", args.argc);

    for (int i = 0; i < args.argc; i++)
    {
        printf("argv[%d] = %s\n", i, args.argv[i]);
    }

    return 0;
}