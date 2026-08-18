#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define RB_SIZE 4

typedef struct
{
    uint8_t buf[RB_SIZE];
    int head;
    int count;
} ring_buffer_t;

bool rbPush(ring_buffer_t *rb, uint8_t data)
{
    if( data == NULL)
    {
        return false;
    }
    // 일단 버퍼가 가득찬건지 확인한다.
    if( rb->count >= RB_SIZE)
    {
        return false;
    }

    rb->buf[rb->head] = data;
    rb->head = (rb->head + 1) % RB_SIZE;
    rb->count ++;

    return true;

}

int main(void)
{
    ring_buffer_t rb = {0};

    printf("push 10 = %d\n", rbPush(&rb, 10));
    printf("push 20 = %d\n", rbPush(&rb, 20));
    printf("push 30 = %d\n", rbPush(&rb, 30));
    printf("push 40 = %d\n", rbPush(&rb, 40));
    printf("push 50 = %d\n", rbPush(&rb, 50));

    printf("head  = %d\n", rb.head);
    printf("count = %d\n", rb.count);

    for (int i = 0; i < RB_SIZE; i++)
    {
        printf("buf[%d] = %d\n", i, rb.buf[i]);
    }

    return 0;
}
