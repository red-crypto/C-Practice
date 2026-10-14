#ifndef FSM_CLI_UI_H
#define FSM_CLI_UI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <windows.h>

typedef struct {
    int32_t id;
    const char *name;
} fsm_cli_command_t;

typedef enum {
    FSM_CLI_OK,
    FSM_CLI_INVALID_ARG,
    FSM_CLI_NOT_READY,
    FSM_CLI_NOT_CONSOLE,
    FSM_CLI_CONSOLE_ERROR,
    FSM_CLI_OUTPUT_ERROR
} fsm_cli_result_t;

typedef enum {
    FSM_CLI_ACTION_NONE,
    FSM_CLI_ACTION_REDRAW,
    FSM_CLI_ACTION_COMMAND,
    FSM_CLI_ACTION_RESET,
    FSM_CLI_ACTION_QUIT
} fsm_cli_action_type_t;

typedef struct {
    fsm_cli_action_type_t type;
    int32_t command_id;
} fsm_cli_action_t;

// Read-only display snapshot. Names must remain valid until Render returns.
typedef struct {
    const char *current_state_name;
    const char *parent_state_name;
    // Optional sub-FSM names. NULL displays "-" until an observer is connected.
    const char *homing_state_name;
    const char *position_state_name;
    const char *pressure_state_name;
    const char *motion_state_name;
    const char *last_command_name;
    const char *last_event_name;
    const char *before_state_name;
    const char *after_state_name;
    const char *result_name;
    uint32_t dispatch_count;
} fsm_cli_view_t;

typedef struct {
    const fsm_cli_command_t *commands;
    size_t command_count;
    size_t selected_index;
    HANDLE input_handle;
    HANDLE output_handle;
    DWORD saved_input_mode;
    DWORD saved_output_mode;
    CONSOLE_CURSOR_INFO saved_cursor;
    bool initialized;
} fsm_cli_t;

// Zero-initialize ui before Init. The command table must outlive the UI session.
fsm_cli_result_t FsmCli_Init(fsm_cli_t *ui, const fsm_cli_command_t *commands,
                            size_t command_count);
// Selection changes only the cursor. Enter returns a command to the application.
fsm_cli_result_t FsmCli_Poll(fsm_cli_t *ui, fsm_cli_action_t *action,
                            uint32_t timeout_ms);
fsm_cli_result_t FsmCli_Render(const fsm_cli_t *ui, const fsm_cli_view_t *view);
const char *FsmCli_CommandName(const fsm_cli_t *ui, int32_t command_id);
fsm_cli_result_t FsmCli_Close(fsm_cli_t *ui);

#endif
