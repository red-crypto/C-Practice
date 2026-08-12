#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
 

typedef struct {
    int (*add)(int a, int b);
    int (*sub)(int a, int b);



} calc_api_t;

int add_impl(int a, int b)
{
    return a+b;
}

int sub_impl(int a, int b)
{
    return a-b;
}

//int calculator()// 틀렷음
calc_api_t *calculator(void)
{
    static calc_api_t api=
    {
        .add = add_impl,
        .sub = sub_impl

    };
    return &api;
}

int main(void)
{
    int sum=0;
    int diff =0;
    sum=calculator()->add(10,5);
    diff=calculator()->sub(10,5);
    printf("sum : %d\r\n", sum);
    printf("dif : %d\r\n", diff);

    printf("1번째 호출 주소: %p\n", (void*)calculator());
    printf("2번째 호출 주소: %p\n", (void*)calculator());
    return 0;
}