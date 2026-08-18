// READ
//Q1. void (*cmd_func)(cli_args_t *); 를 해석해라.
//A1. 일단은 함수 포인터 형태이고, cmd_func라는 이름으로 함수의 주소값을 가리키면서, 인자로는 cli_args_t *를 인자로 받아서 전달, 리턴은 void.
//    여기서 함수 포인터를 쓴 이유는? 위에서 설명한것처럼 함수의 주소값을 가리키니까 포인터로 직접 주소에 연결하는 방식, 전역 메모리에 수정이 가능해짐?

//Q2. cli_cmd_t *p_cmd;
//A2. 이거는 포인터로 선언한 이유가, cli_cmd_t의 구조체 변수를 포인터로 선언함으로써 전역에서 동일한 주소에 접근하겠다는 의도임.

//Q3. void(*handlers[4])(cli_args_t *);
//A3. 이것도 함수 포인터일텐데, 배열형태로 handlers[0], [1], [2], [3] 마다 다른건가? 무슨 의미이지

// PREDICT
/*
#include <stdio.h>

typedef void (*cmd_func_t)(int);

void led(int n)   { printf("LED %d\n", n); }
void motor(int n) { printf("MOTOR %d\n", n); }

int main(void)
{
    cmd_func_t funcs[2] = {led, motor};
    cmd_func_t *p = funcs;

    p[1](100);
    (*p)(3);
}
*/
// MOTOR 100
// (*p)(3); 을 모르겠네, LED 3 / MOTOR 3이 되는건지
// p[0] = led 함수, *p = 위에 선언은 포인터p를 funcs로 한다는 의미이고, 아래애서의 *p(3)은 그 주소의 값이니까.. 함수자체를 말하는건가 그래서 동일하다?

// Build
// 오늘은 실제 CLI의 cliRunCmd()를 아주 작게 축소한다
#include <stdio.h>
#include <string.h>

typedef struct
{
    int argc;
    char **argv;
} cli_args_t;


typedef struct
{
    const char *cmd_str;
    void (*cmd_func)(cli_args_t *);
} cli_cmd_t;


void cmdLed(cli_args_t *args)
{
    printf("LED argc=%d\n", args->argc);
}


void cmdMotor(cli_args_t *args)
{
    printf("MOTOR argc=%d\n", args->argc);
}


void cmdHelp(cli_args_t *args)
{
    printf("HELP argc=%d\n", args->argc);
}
//반환값
//명령을 찾고 실행했으면:1 아니면 :0

int cliRunCmd(const char *cmd,
              cli_args_t *args,
              const cli_cmd_t *cmd_table,
              int cmd_count)
{
    // TODO
    // 입력된 문자열을 하나씩 cmd_table의 값과 비교를 해야한다.
    for(int i=0 ; i< cmd_count;i++)
    {

        if(strcmp(cmd_table[i].cmd_str,cmd) == 0)
        {
            static char *dummy_argv[1];
            dummy_argv[0] = (char *)cmd_table[i].cmd_str;
            
            args->argv = dummy_argv;
            args->argc = i;

            cmd_table[i].cmd_func(args);

            return 1;
        }

    }
    return 0;
}

int main(void)
{
    cli_cmd_t cmd_table[] =
    {
        {"led",   cmdLed},
        {"motor", cmdMotor},
        {"help",  cmdHelp}
    };

    cli_args_t args;
    args.argc = 2;
    args.argv = NULL;

    int ret;

    ret = cliRunCmd("motor",
                    &args,
                    cmd_table,
                    3);

    printf("ret=%d\n", ret);


    ret = cliRunCmd("abc",
                    &args,
                    cmd_table,
                    3);

    printf("ret=%d\n", ret);

    return 0;
}

