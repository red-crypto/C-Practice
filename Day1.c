#include <stdio.h>
#include <string.h>

typedef struct
{
    int argc;
    char **argv;
} cli_args_t;


typedef struct
{
    char cmd_str[16];
    void (*cmd_func)(cli_args_t *);
} cli_cmd_t;


void cmdLed(cli_args_t *args)
{
    printf("LED argc = %d\n", args->argc);
    if( args->argc < 1 )
    {
        printf("Usage : led on|off\r\n");
    }

    if (strcmp(args->argv[0],"off") == 0)
    {
        printf("LED OFF\r\n");
        // GPIO_Write(..., 0);
    }
}


void cliAdd(cli_cmd_t *cmd,
            const char *cmd_str,
            void (*func)(cli_args_t *args))
{
    // TODO 1
    // cmd_str을 cmd->cmd_str에 저장
    strcpy(cmd->cmd_str, cmd_str);
    

    // TODO 2
    // 함수 주소를 cmd->cmd_func에 저장
    //cmd->cmd_func = func;
    args->argv = func;
}


int main(void)
{
    cli_cmd_t cmd;
    cli_args_t args = {2};

    // TODO 3
    // cliAdd()를 호출하여
    // "led"와 cmdLed를 cmd에 등록
    cliAdd(&cmd, "led", cmdLed);

    

    printf("command = %s\n", cmd.cmd_str);

    // TODO 4
    // 등록된 함수를 실행
    cmd.cmd_func(&args);

    

    return 0;
}
