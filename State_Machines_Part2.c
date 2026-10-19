#include <stdbool.h>
#include <stdio.h>

// 일단 나만의 방식으로 적어봐
typedef struct
{
    enum{
    INIT_SIG,
    TIMEOUT_SIG,
    BUTTON_PRESSED_SIG,
    BUTTON_RELEASED_SIG
    } sig;

} Event;

typedef struct{
    enum{
        WAIT4BUTTON_STATE,
        BLINK_STATE,
        PAUSE_STATE,
        BOOM_STATE
    } state;
 
    // TimeEvent Event;
} TimeBomb;

void ledGreenOn(void)
{
    //GPIO_Write(LED_GREEN, HIGH);
}


static void Timebomb_dispatch(TimeBomb *me, Event *e)
{
    if(e->sig == INIT_SIG)
    {
        ledGreenOn();
        me->state = WAIT4BUTTON_STATE;
    }
    switch(me->state)
    {
        case WAIT4BUTTON_STATE:
            switch(e->sig)
            {
                case BUTTON_PRESSED_SIG:
                    me->state = BLINK_STATE;//원래는 이러면 안될텐데
                break;
                default:
                break;

            }
        break;

        case BLINK_STATE:
            //3번 깜빡이고 <-?
        break;

        case PAUSE_STATE:

        break;

        case BOOM_STATE:
            // PAUSE에서 넘어올때 실행해버리고,
            // BOOM_STATE는 그냥 break; 해버림
        break;
        

    }




}

int main(void){

    //






    return 0;
}

