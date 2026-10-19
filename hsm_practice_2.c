#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>


typedef enum 
{
    ST_NONE = -1,
    ST_TOP,
    ST_OFF,
    ST_ON,
    ST_SOLID,
    ST_BLINK
} state_t;


typedef enum
{
    H_HANDLED,
    H_SUPER,
    H_TRAN,
    H_REJECTED
} handle_result_t;


typedef enum
{
    EV_SHORT,
    EV_LONG,
    EV_TIMEOUT,
    EV_RESET
} event_t;

typedef struct 
{
    state_t state;
    bool led_on;
} led_hsm_t;



static const char *StateName(state_t s)
{
    switch (s) {
    case ST_TOP:   return "TOP";
    case ST_OFF:   return "OFF";
    case ST_ON:    return "ON";
    case ST_SOLID: return "SOLID";
    case ST_BLINK: return "BLINK";
    default:       return "NONE";
    }
}
static state_t Parent(state_t state)
{
    switch(state){

        case ST_SOLID:
        case ST_BLINK:
            return ST_ON;

        case ST_ON:
        case ST_OFF:
            return ST_TOP;

        case ST_TOP:
            return ST_NONE;
        
        default :
            return ST_NONE;

    }

    return ST_NONE;
}


handle_result_t HandleState(
    led_hsm_t *h,
    state_t state,
    event_t event,
    state_t *target
)
{

    switch(state)
    {
        case ST_OFF:
            switch(event)
            {
                case EV_SHORT:
                    *target = ST_SOLID; // 다음에 커맨드 처리
                    return H_TRAN;
                
                default:
                    return H_SUPER;
            }
            break;
        case ST_SOLID:
            // TODO 1
            // EV_SHORT 발생 시 어떻게 해야 할까?
            switch(event)
            {
                case EV_SHORT:
                    *target = ST_BLINK;
                    return H_TRAN;
                default:
                    return H_SUPER;

            }
            break;

        case ST_BLINK:
            // TODO 2
            // EV_SHORT?
            // EV_TIMEOUT?
            switch(event)
            {
                case EV_SHORT:
                    *target = ST_SOLID;
                    return H_TRAN;
                case EV_TIMEOUT:
                    //timeout시 상태변화는 없고 LED만 깜빡인다.
                    // h->state = ? ; 이걸 어떻게 led_on 으로 바꾸나?
                    h->led_on = !h->led_on;
                    return H_HANDLED;
                    // 상태를 변경하지 않고, 이벤트만 처리하면 HANDLED!
                default:
                    return H_SUPER;
            }
            break;

        case ST_ON:
            // 공통 부모 상태의 이벤트 처리
            // TODO 3
            // EV_LONG?
            switch(event)
            {
                case EV_LONG:
                    *target = ST_OFF;
                    return H_TRAN;

            }

            break;

        case ST_TOP:
            // 최상위 상태의 이벤트 처리
            // TODO 4
            // EV_RESET?
            switch(event)
            {
                case EV_RESET:
                    *target = ST_OFF;
                    return H_TRAN;
            }            
            break;
    }

    return H_SUPER;
}
// IsAncestor(a,b) a가 b의 조상 또는 자기 자신인가?
// IsAncestor(ST_ON, ST_BLINK) == TRUE;
static bool IsAncestor(state_t ancestor, state_t state)
{
    // a가 b의 조상 또는 자기 자신이면 TRUE 를 리턴한다.
    // 반복해서 Parent의 리턴값이 -1(ST_NONE)까지 도달하는 경우, while에서 탈출하고 return FALSE;
    state_t ret = state;

    while( ret != ST_NONE)
    {

        if(ret == ancestor)
        {
            return true;
        }
        ret = Parent(ret);
    }

    return false;
}

// 최소공배수처럼 최소 조상을 찾는다 
// 어떻게 IsAncestor를 통해서 최소조상을 찾을 수 있을까?

state_t FindLca(state_t source, state_t target)
{
    state_t cur = source;
    while( cur != ST_NONE )
    {
        if(IsAncestor(cur,target) != 0)
        {
            return cur;
        }
        cur = Parent(cur);
    }
    return ST_NONE;
}


// 내가 고민하던 그래서 언제 켜야하는데? 에 대한 답변은
// Entry의 실행과 함께 동작하는것으로 정하는게 좋다
// 이유 : 상태 전이 알고리즘이 LED라는 별개 하드웨어를 알 필요가 없기 때문.





// HSM의 상태가 source(ST_BLINK)에서 target(ST_OFF)으로 전이한다.
// ST_BLINK -> ST_OFF, LCA == ST_TOP
//

void Hsm_Transition()
{

}

void Hsm_Dispatch(led_hsm_t *h, event_t event)
{
    state_t s = h->state;

    while(s != ST_NONE)
    {
        state_t target = ST_NONE;

        handle_result_t result =
            HandleState(h, s, event, &target);
        printf("Handling at: %s\n", StateName(s));
        if (result == H_SUPER)
        {
            // 부모 상대로 탐색 대상을 변경
            // 탐색 = search, 
            s = Parent(s);
        }
        if (result == H_TRAN)
        {
            printf("Transition Request: %s -> %s\n",
                StateName(h->state),
                StateName(target));
            return ;
        }
        if (result == H_HANDLED)
        {
            return ;
        }
        if (result == H_REJECTED)
        {
            return ;
        }


    }



}



int main(void)
{
    led_hsm_t hsm = {
        .state = ST_BLINK
    };

    Hsm_Dispatch(&hsm, EV_LONG);

    printf("CURRENT: %s\n", StateName(hsm.state));

    state_t ret;

    ret = FindLca(ST_BLINK,ST_SOLID);

    printf("FindLca: %s\n", StateName(ret));


    return 0;
}

