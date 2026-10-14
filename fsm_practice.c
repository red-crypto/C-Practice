#include "fsm_practice.h"

#include <stddef.h>
#include <windows.h>
#include <stdbool.h>
// [3] PRACTICE: reset all owned state, then choose the initial state.
fsm_result_t Fsm_Init(fsm_t *fsm)
{
    if (fsm == NULL)
    {
        return RST_INVALID_ARG;
    }
    *fsm = (fsm_t){ .state = STATE_IDLE };
    return RST_OK;
}
static volatile int g_traj_cnt   = 1000;
static volatile int g_homing_cnt = 1000;

static ULONGLONG s_timer_ms;
static bool s_timer_started;

static ULONGLONG s_traj_start_ms;
static bool s_traj_started;

static ULONGLONG s_homing_start_ms;
static bool s_homing_started;

void timer_start(void)
{
    s_timer_ms = GetTickCount64();
    s_timer_started = true;
}

bool timer_is_done(void)
{
    return s_timer_started &&
           ((GetTickCount64() - s_timer_ms) >= 500ULL);
}

void traj_start(void)
{
    s_traj_start_ms = GetTickCount64();
    s_traj_started = true;
}

bool traj_is_done(void)
{
    return s_traj_started &&
           ((GetTickCount64() - s_traj_start_ms) >= 2000ULL);
}

void homing_start(void)
{
    s_traj_start_ms = GetTickCount64();
    s_traj_started = true;
}
bool homing_is_done(void)
{
    return s_traj_started &&
           ((GetTickCount64() - s_traj_start_ms) >= 1000ULL);
}
bool homing_is_not_vaild(void)
{
    return true;

}
void motion_start(fsm_t *fsm)
{
    traj_start();
    timer_start();
    fsm->motion.motion_e = MOT_ACCEL;
}
void motion_fsm(fsm_t *fsm, event_t event)
{
    motion_t *mot = &fsm->motion;

    switch(mot->motion_e)
    {
        case MOT_IDLE:
            // event를 외부에서 실행해도 되는건가?
        break;

        case MOT_ACCEL:
            if(timer_is_done())
            {
                mot->motion_e = MOT_RUN;
                timer_start();
            }
        break;

        case MOT_RUN:
            if(timer_is_done())
            {
                mot->motion_e = MOT_DECEL;
                timer_start();
            }
        break;

        case MOT_DECEL:
            if(traj_is_done())
            {
                mot->motion_e = MOT_IDLE;
            }
        break;


        default:
        break;

    }


}

void homing_fsm(fsm_t *fsm, event_t event)
{
    // event_t event;
    switch(fsm->s_home.state)
    {
        case HOME_IDLE:
            // guard 조건을 만족시키면 커맨드에서 home이 들어오면 home_search로 전이한다.
            if(event == EV_HOMING)
            {
                //guard를 좀 더 세분화 해서 블락해야할듯
                if(!homing_is_not_vaild())
                {
                    fsm->state = STATE_FAULT;
                    break;
                }

                //... guard를 모두 뚫고 진입하게 된다면,
                    

                fsm->s_home.state = HOME_START;
            }
        break;  
        case HOME_START:
            //자동이행? 그럼 왜 굳이 state를 나눠야할까?
            //어떻게 다른곳에서 옮기지?
            motion_start(fsm);

            fsm->s_home.state= HOME_SEARCH;


        break;
        case HOME_SEARCH:
            //traj. 호출해서 진행... 이것도 별도의 FSM을 구축해서 돌리긴 해야할듯

            // 내부에서는 moveTo같은걸로?
            // 아니면 일단 단순하게 ..
            if(traj_is_done())
            {
                fsm->s_home.state= HOME_DONE;

            }

        break;

        case HOME_DONE:
            if(event == EV_IDLE)
            {
                fsm->s_home.state = HOME_IDLE;
                
            // return으로 지속적으로 dispatcher에게 상태를 전달해야함..?
            }

        break;

        default:
            //fsm->state = STATE_FAULT를 하지 않고, 상태만 전달해서 Fsm_Dispatch에서 직접 상태전이하도록 만든다.
        break;
    
    }
}






// [4] MAIN PRACTICE AREA: implement YOUR FSM / HSM here.
fsm_result_t Fsm_Dispatch(fsm_t *fsm, event_t event)
{
    if ((fsm == NULL) || (event <= EV_NONE) || (event >= EV_COUNT))
    {
        return RST_INVALID_ARG;
    }
    

    // TODO: inspect fsm->state, handle event, evaluate guards, and transition.
    // FSM: select the current-state branch, then its event branch.
    // HSM: try the leaf handler first; pass unhandled events to its parent.
    // Put exit/entry actions here or in helpers owned by this module.
    // Return FSM_OK, FSM_UNHANDLED, or FSM_REJECTED to expose the outcome.
    // No command-ID cast, keyboard input, Windows API, or printf belongs here.

    if(event == EV_RESTORE)
    {
        Fsm_Init(fsm);
    }

    switch(fsm->state)
    {
        case STATE_IDLE:
            switch(event)
                {
                    case EV_HOMING:
                        fsm->state = STATE_HOMING;
                        return RST_OK;
                    default :
                        return RST_UNHANDLED;
                }

        break;
        case STATE_HOMING:
                //동작조건 확인을 위한 guard 
                // if(!homing_is_done())
                // {
                //     break;
                // }

                switch(event)
                {
                    case EV_FAULT:
                        fsm->state = STATE_FAULT;
                        return RST_OK;
                    case EV_IDLE:
                        fsm->state = STATE_IDLE;
                        return RST_OK;
                    default:
                        return RST_UNHANDLED;
                    break;
                }

        break;
        case STATE_POSITION:

        break;
        case STATE_PRESSURE:

        break;
        case STATE_FAULT:

        break;

        


        default :
        //존재하면 안되는 구간 ESTOP처럼 처리
        
        break;
    }


    return RST_UNHANDLED;
}









// [5] OBSERVATION: extend these names when adding states or events.
const char *Fsm_StateName(state_t state)
{
    switch (state)
    {
        case STATE_IDLE:  return "IDLE";
        case STATE_HOMING:   return "HOME";
        case STATE_POSITION:  return "POSITION";
        case STATE_PRESSURE:  return "PRESSURE";
        case STATE_FAULT: return "FAULT";
        default:    return "INVALID";
    }
}
const char *Homing_StateName(home_e state)
{
    switch (state)
    {
        case HOME_IDLE:  return "IDLE";
        case HOME_SEARCH:   return "SEARCH";
        case HOME_DONE:  return "DONE";
        default:    return "INVALID";
    }
}
const char *Motion_StateName(motion_e state)
{
    switch (state)
    {
        case MOT_IDLE:  return "IDLE";
        case MOT_ACCEL:  return "ACCEL";
        case MOT_RUN:  return "RUN";
        case MOT_DECEL:  return "DECEL";
        default:    return "INVALID";
    }
}


const char *Fsm_ParentStateName(const fsm_t *fsm)
{
    // HSM practice: map your parent/super-state here once you add it to fsm_t.
    (void)fsm;
    return "-";
}

const char *Fsm_EventName(event_t event)
{
    switch (event)
    {
        case EV_NONE:         return "NONE";
        case EV_IDLE:         return "IDLE_REQUEST";
        case EV_HOMING:         return "HOME REQUEST";
        case EV_POSITION:     return "POSITION REQUEST";
        case EV_PRESSURE:     return "PRESSURE REQUEST";
        case EV_FAULT:        return "FAULT";
        default:              return "INVALID";
    }
}

const char *Fsm_ResultName(fsm_result_t result)
{
    switch (result)
    {
        case RST_OK:          return "HANDLED";
        case RST_UNHANDLED:   return "UNHANDLED";
        case RST_REJECTED:    return "REJECTED";
        case RST_INVALID_ARG: return "INVALID_ARG";
        default:              return "INVALID";
    }
}
