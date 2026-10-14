#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
// 2. 상태 전이 전용 함수 (Next State Function)
typedef enum{
    GREEN,
    YELLOW,
    RED
}state_t;

typedef enum{
    EVENT_NONE,
    STOP
}event_t;

typedef struct{
    state_t state;
    event_t event;
    uint16_t green_on;
}fsm_t;

fsm_t fsm = {
    .state = GREEN,
    .event = EVENT_NONE,
    .green_on = 0U
};


void call_next_state(void) {
    switch (fsm.state) {
        case GREEN:  fsm.state = YELLOW; break;
        case YELLOW: fsm.state = RED;    break;
        case RED:    fsm.state = GREEN;  break;
    }
}
// 이건 나중에 하위모듈들 led.c, button.c ... 로 구현되는 부분들

void turn_on_green(void)
{
fsm.green_on = 1;
}
void turn_off_green(void)
{
fsm.green_on = 0;
}

// ----------------------------------------------------------


static void fsm_update(void)
{
    if (fsm.event == STOP)
    {
        call_next_state();
    }
    // Consume each event once, even if this function is called again.
    fsm.event = EVENT_NONE;

    switch (fsm.state)
    {
        case GREEN:
            if (!fsm.green_on) turn_on_green();
            break;
        case YELLOW:
        case RED:
            if (fsm.green_on) turn_off_green();
            break;
    }
}

// 이건 그냥 상태 관측용
static void print_fsm_state(void)
{
    const char *state_name = "INVALID";

    switch (fsm.state)
    {
        case GREEN:  state_name = "GREEN";  break;
        case YELLOW: state_name = "YELLOW"; break;
        case RED:    state_name = "RED";    break;
    }

    printf("state=%s, green_on=%u\n", state_name,
           (unsigned int)fsm.green_on);
    fflush(stdout);
}
//

int main(void)
{
    int key;

    fsm_update();
    puts("s + Enter: STOP event / q + Enter: quit");
    print_fsm_state();

    // getchar() returns int so that EOF remains distinct from character values.
    while ((key = getchar()) != EOF)
    {
        if ((key == '\n') || (key == '\r'))
        {
            continue;
        }

        if (key == 'q')
        {
            break;
        }

        if (key == 's')
        {
            fsm.event = STOP;
            puts("Pressed 's'");
            //fflush(stdout);
            continue;
        }
        if (key == 'h')
        {
            puts("Homing Start\r\n");
            //fflush(stdout);
            continue;
        }
        if (key == 'p')
        {
            puts("Position");
            //fflush(stdout);
            continue;
        }

        fsm_update();
        print_fsm_state();
    }




    if (ferror(stdin))
    {
        perror("stdin");
        return 1;
    }

    return 0;
}
