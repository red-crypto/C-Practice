#include "fsm_cli_ui.h"
#include "fsm_practice.h"

#include <stdio.h>

enum { UI_REFRESH_PERIOD_MS = 100 };

// [6] INPUT REGISTRATION: menu IDs are commands, never internal state values.
typedef enum {
    CMD_IDLE = 100,
    CMD_HOMING,
    CMD_POSITION,
    CMD_PRESSURE,
    CMD_FAULT,
    CMD_RESTORE
} command_t;

static const fsm_cli_command_t command_menu[] = {
    { CMD_IDLE,  "IDLE" },
    { CMD_HOMING,   "HOME" },
    { CMD_FAULT, "FAULT" },
    { CMD_PRESSURE, "PRESSURE" },
    { CMD_POSITION,  "POSITION" },
    { CMD_RESTORE,  "RESTORE" }
};

// [7] INPUT ADAPTER: add a command-to-event mapping when registering a command.
static bool App_CommandToEvent(int32_t command_id, event_t *event)
{
    if (event == NULL)
    {
        return false;
    }
    switch (command_id)
    {
        case CMD_IDLE:  *event = EV_IDLE; return true;
        case CMD_HOMING:   *event = EV_HOMING;  return true;
        case CMD_FAULT: *event = EV_FAULT;        return true;
        case CMD_PRESSURE:  *event = EV_PRESSURE;         return true;
        case CMD_POSITION: *event = EV_POSITION;      return true;
        case CMD_RESTORE: *event = EV_RESTORE;      return true;
      
        default:       *event = EV_NONE;         return false;
    }
}

// HARNESS: captures evidence without deciding whether a transition is allowed.
typedef struct {
    int32_t command_id;
    event_t event;
    state_t before_state;
    state_t after_state;
    fsm_result_t result;
    uint32_t dispatch_count;
} app_trace_t;

static void App_DispatchCommand(fsm_t *fsm, int32_t command_id,
                                app_trace_t *trace)
{
    trace->command_id = command_id;
    trace->before_state = fsm->state;
    if (trace->dispatch_count < UINT32_MAX)
    {
        ++trace->dispatch_count;
    }
    if (App_CommandToEvent(command_id, &trace->event))
    {
        // Only the practice module decides the next state.
        trace->result = Fsm_Dispatch(fsm, trace->event);
    }
    else
    {
        trace->result = RST_INVALID_ARG;
    }
    trace->after_state = fsm->state;
}

static fsm_cli_view_t App_BuildView(const fsm_cli_t *ui, const fsm_t *fsm,
                                  const app_trace_t *trace)
{
    const bool has_event = (trace->dispatch_count != 0U);
    return (fsm_cli_view_t){
        .current_state_name = Fsm_StateName(fsm->state),
        .parent_state_name = Fsm_ParentStateName(fsm),
        // Connect read-only state-name getters here as you add each sub-FSM.
        // These expressions must observe state without advancing an FSM.
        .homing_state_name = Homing_StateName(fsm->s_home.state),
        .position_state_name = NULL,
        .pressure_state_name = NULL,
        .motion_state_name = Motion_StateName(fsm->motion.motion_e),
        .last_command_name = has_event
                           ? FsmCli_CommandName(ui, trace->command_id) : "-",
        .last_event_name = has_event ? Fsm_EventName(trace->event) : "-",
        .before_state_name = has_event
                           ? Fsm_StateName(trace->before_state) : "-",
        .after_state_name = has_event
                          ? Fsm_StateName(trace->after_state) : "-",
        .result_name = has_event ? Fsm_ResultName(trace->result) : "-",
        .dispatch_count = trace->dispatch_count
    };
}

// HARNESS LOOP: keyboard -> command -> event -> your FSM -> read-only view.
int main(void)
{
    fsm_t fsm = {0};
    fsm_cli_t ui = {0};
    app_trace_t trace = {0};
    const size_t command_count = sizeof(command_menu) / sizeof(command_menu[0]);

    fsm_result_t init_result = Fsm_Init(&fsm);
    if (init_result != RST_OK)
    {
        fputs("FSM initialization failed.\n", stderr);
        return 1;
    }
    fsm_cli_result_t result = FsmCli_Init(&ui, command_menu, command_count);
    if (result != FSM_CLI_OK)
    {
        fprintf(stderr, "UI init failed (code=%d). Run in a Windows terminal.\n",
                (int)result);
        return 1;
    }

    fsm_cli_view_t view = App_BuildView(&ui, &fsm, &trace);
    result = FsmCli_Render(&ui, &view);
    ULONGLONG last_render_ms = GetTickCount64();
    while (result == FSM_CLI_OK)
    {
        event_t cycle_event = EV_NONE;
        fsm_cli_action_t action;
        result = FsmCli_Poll(&ui, &action, 20U);
        if ((result != FSM_CLI_OK) || (action.type == FSM_CLI_ACTION_QUIT))
        {
            break;
        }
        if (action.type == FSM_CLI_ACTION_COMMAND)
        {
            App_DispatchCommand(&fsm, action.command_id, &trace);
            cycle_event = trace.event;
        }
        else if (action.type == FSM_CLI_ACTION_RESET)
        {
            init_result = Fsm_Init(&fsm);
            if (init_result != RST_OK)
            {
                break;
            }
            // Restart a test from your initialization policy and a clean trace.
            trace = (app_trace_t){0};
        }
        // sub-module들 동작시키는 구간?
        
        homing_fsm(&fsm, cycle_event);
        motion_fsm(&fsm, cycle_event);
        //

        const ULONGLONG now_ms = GetTickCount64();
        if ((action.type != FSM_CLI_ACTION_NONE) ||
            ((now_ms - last_render_ms) >= UI_REFRESH_PERIOD_MS))
        {
            view = App_BuildView(&ui, &fsm, &trace);
            result = FsmCli_Render(&ui, &view);
            last_render_ms = now_ms;
        }

    }

    const fsm_cli_result_t cleanup_result = FsmCli_Close(&ui);
    if ((result != FSM_CLI_OK) || (cleanup_result != FSM_CLI_OK) ||
        (init_result != RST_OK))
    {
        fprintf(stderr, "Harness failed (ui=%d, cleanup=%d, init=%d).\n",
                (int)result, (int)cleanup_result, (int)init_result);
        return 1;
    }
    puts("FSM terminal closed.");
    return 0;
}
