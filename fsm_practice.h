#ifndef FSM_PRACTICE_H
#define FSM_PRACTICE_H
typedef enum
{
    HOME_IDLE =0,
    HOME_START,
    HOME_SEARCH,
    HOME_DONE
} home_e;
typedef struct
{
    home_e state;
} home_t;

typedef enum
{
    MOT_IDLE=0,
    MOT_ACCEL,
    MOT_RUN,
    MOT_DECEL,
    
} motion_e;
typedef struct{
    motion_e motion_e;

} motion_t;

// [1] PRACTICE: internal states and incoming events are separate concepts.
typedef enum {
    STATE_IDLE,
    STATE_HOMING,
    STATE_POSITION,
    STATE_PRESSURE,
    STATE_FAULT
} state_t;

typedef enum {
    EV_NONE,
    EV_IDLE,
    EV_HOMING,
    EV_POSITION,
    EV_PRESSURE,
    EV_FAULT,
    EV_RESTORE,
    EV_COUNT
} event_t;

typedef enum {
    RST_OK,          // Event handled; a state change is not required.
    RST_UNHANDLED,   // No handler for this event in the current state.
    RST_REJECTED,    // Event recognized, but a guard prevents handling it.
    RST_INVALID_ARG
} fsm_result_t;

// [2] PRACTICE: add guards, counters, or HSM parent/child state here.
// The console harness and your FSM run in MAIN; no volatile is needed.
typedef struct {
    state_t state;
    home_t s_home;
    motion_t motion;
} fsm_t;


fsm_result_t Fsm_Init(fsm_t *fsm);
fsm_result_t Fsm_Dispatch(fsm_t *fsm, event_t event);
void homing_fsm(fsm_t *fsm, event_t event);
void motion_fsm(fsm_t *fsm, event_t event);
// Observation functions must not change the FSM or print to the terminal.
const char *Fsm_StateName(state_t state);
const char *Homing_StateName(home_e state);
const char *Motion_StateName(motion_e state);

const char *Fsm_ParentStateName(const fsm_t *fsm);
const char *Fsm_EventName(event_t event);
const char *Fsm_ResultName(fsm_result_t result);

#endif
