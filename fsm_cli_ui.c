#include "fsm_cli_ui.h"

#include <stdio.h>

enum { MENU_VISIBLE_ROWS = 8, VIEW_FIXED_ROWS = 19 };

const char *FsmCli_CommandName(const fsm_cli_t *ui, int32_t command_id)
{
    if ((ui == NULL) || !ui->initialized)
    {
        return "UNKNOWN";
    }
    for (size_t index = 0U; index < ui->command_count; ++index)
    {
        if (ui->commands[index].id == command_id)
        {
            return ui->commands[index].name;
        }
    }
    return "UNKNOWN";
}

fsm_cli_result_t FsmCli_Init(fsm_cli_t *ui, const fsm_cli_command_t *commands,
                            size_t command_count)
{
    if ((ui == NULL) || (commands == NULL) || (command_count == 0U))
    {
        return FSM_CLI_INVALID_ARG;
    }
    if (ui->initialized)
    {
        return FSM_CLI_INVALID_ARG;
    }
    for (size_t index = 0U; index < command_count; ++index)
    {
        if ((commands[index].name == NULL) || (commands[index].name[0] == '\0'))
        {
            return FSM_CLI_INVALID_ARG;
        }
        for (size_t previous = 0U; previous < index; ++previous)
        {
            if (commands[previous].id == commands[index].id)
            {
                return FSM_CLI_INVALID_ARG;
            }
        }
    }

    ui->input_handle = GetStdHandle(STD_INPUT_HANDLE);
    ui->output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleMode(ui->input_handle, &ui->saved_input_mode) ||
        !GetConsoleMode(ui->output_handle, &ui->saved_output_mode))
    {
        return FSM_CLI_NOT_CONSOLE;
    }
    if (!GetConsoleCursorInfo(ui->output_handle, &ui->saved_cursor))
    {
        return FSM_CLI_CONSOLE_ERROR;
    }

    // Raw key events allow immediate navigation and graceful Ctrl+C cleanup.
    if (!SetConsoleMode(ui->input_handle,
                        ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT))
    {
        return FSM_CLI_CONSOLE_ERROR;
    }
    if (!SetConsoleMode(ui->output_handle, ui->saved_output_mode |
                        ENABLE_PROCESSED_OUTPUT |
                        ENABLE_VIRTUAL_TERMINAL_PROCESSING))
    {
        if (!SetConsoleMode(ui->input_handle, ui->saved_input_mode))
        {
            return FSM_CLI_CONSOLE_ERROR;
        }
        return FSM_CLI_CONSOLE_ERROR;
    }

    ui->commands = commands;
    ui->command_count = command_count;
    ui->selected_index = 0U;
    ui->initialized = true;

    // The alternate screen preserves the terminal contents when we exit.
    if ((fputs("\x1b[?1049h\x1b[?25l\x1b[2J\x1b[H", stdout) == EOF) ||
        (fflush(stdout) == EOF))
    {
        const fsm_cli_result_t cleanup_result = FsmCli_Close(ui);
        return (cleanup_result == FSM_CLI_OK) ? FSM_CLI_OUTPUT_ERROR
                                              : cleanup_result;
    }
    return FSM_CLI_OK;
}

fsm_cli_result_t FsmCli_Poll(fsm_cli_t *ui, fsm_cli_action_t *action,
                            uint32_t timeout_ms)
{
    INPUT_RECORD input;
    DWORD records_read = 0U;
    DWORD wait_result;

    if ((ui == NULL) || (action == NULL) || (timeout_ms == UINT32_MAX))
    {
        return FSM_CLI_INVALID_ARG;
    }
    *action = (fsm_cli_action_t){ .type = FSM_CLI_ACTION_NONE };
    if (!ui->initialized)
    {
        return FSM_CLI_NOT_READY;
    }

    wait_result = WaitForSingleObject(ui->input_handle, (DWORD)timeout_ms);
    if (wait_result == WAIT_TIMEOUT)
    {
        return FSM_CLI_OK;
    }
    if ((wait_result != WAIT_OBJECT_0) ||
        !ReadConsoleInputW(ui->input_handle, &input, 1U, &records_read) ||
        (records_read != 1U))
    {
        return FSM_CLI_CONSOLE_ERROR;
    }
    if (input.EventType == WINDOW_BUFFER_SIZE_EVENT)
    {
        action->type = FSM_CLI_ACTION_REDRAW;
        return FSM_CLI_OK;
    }
    if ((input.EventType != KEY_EVENT) || !input.Event.KeyEvent.bKeyDown)
    {
        return FSM_CLI_OK;
    }

    const WORD key = input.Event.KeyEvent.wVirtualKeyCode;
    const DWORD modifiers = input.Event.KeyEvent.dwControlKeyState;
    if ((key == VK_ESCAPE) || (key == 'Q') ||
        ((key == 'C') &&
         ((modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0U)))
    {
        action->type = FSM_CLI_ACTION_QUIT;
    }
    else if (key == 'R')
    {
        action->type = FSM_CLI_ACTION_RESET;
    }
    else if ((key == VK_UP) || (key == 'W'))
    {
        ui->selected_index = (ui->selected_index == 0U)
                           ? ui->command_count - 1U : ui->selected_index - 1U;
        action->type = FSM_CLI_ACTION_REDRAW;
    }
    else if ((key == VK_DOWN) || (key == 'S'))
    {
        ui->selected_index = (ui->selected_index + 1U == ui->command_count)
                           ? 0U : ui->selected_index + 1U;
        action->type = FSM_CLI_ACTION_REDRAW;
    }
    else if (key == VK_RETURN)
    {
        action->type = FSM_CLI_ACTION_COMMAND;
        action->command_id = ui->commands[ui->selected_index].id;
    }
    return FSM_CLI_OK;
}

fsm_cli_result_t FsmCli_Render(const fsm_cli_t *ui, const fsm_cli_view_t *view)
{
    if ((ui == NULL) || (view == NULL) ||
        (view->current_state_name == NULL) || (view->parent_state_name == NULL) ||
        (view->last_command_name == NULL) || (view->last_event_name == NULL) ||
        (view->before_state_name == NULL) || (view->after_state_name == NULL) ||
        (view->result_name == NULL))
    {
        return FSM_CLI_INVALID_ARG;
    }
    if (!ui->initialized)
    {
        return FSM_CLI_NOT_READY;
    }

    // Reserve a final row so redraws do not scroll a short terminal window.
    size_t visible_rows = 4U;
    CONSOLE_SCREEN_BUFFER_INFO console_info;
    if (GetConsoleScreenBufferInfo(ui->output_handle, &console_info))
    {
        const int window_rows = console_info.srWindow.Bottom -
                                console_info.srWindow.Top + 1;
        visible_rows = (window_rows > (VIEW_FIXED_ROWS + 1))
                     ? (size_t)(window_rows - VIEW_FIXED_ROWS - 1) : 1U;
        if (visible_rows > MENU_VISIBLE_ROWS)
        {
            visible_rows = MENU_VISIBLE_ROWS;
        }
    }

    const size_t first_visible =
        (ui->selected_index / visible_rows) * visible_rows;
    size_t rows = ui->command_count - first_visible;
    if (rows > visible_rows)
    {
        rows = visible_rows;
    }

    // Redraw at a fixed position; clear each row to remove stale longer text.
    printf("\x1b[H\x1b[2K\x1b[1;36mFSM / HSM PRACTICE TERMINAL\x1b[0m\n");
    printf("\x1b[2K----------------------------------------------------------\n");
    printf("\x1b[2KSupervisor : \x1b[1;32m%.32s\x1b[0m\n",
           view->current_state_name);
    printf("\x1b[2KHoming     : %.32s\n",
           (view->homing_state_name != NULL) ? view->homing_state_name : "-");
    printf("\x1b[2KPosition   : %.32s\n",
           (view->position_state_name != NULL) ? view->position_state_name : "-");
    printf("\x1b[2KPressure   : %.32s\n",
           (view->pressure_state_name != NULL) ? view->pressure_state_name : "-");
    printf("\x1b[2KMotion     : %.32s\n",
           (view->motion_state_name != NULL) ? view->motion_state_name : "-");
    printf("\x1b[2KParent     : %.32s\n", view->parent_state_name);
    printf("\x1b[2KSelected   : %.32s (Enter sends command)\n",
           ui->commands[ui->selected_index].name);
    printf("\x1b[2KCommand    : %.32s (#%lu)\n", view->last_command_name,
           (unsigned long)view->dispatch_count);
    printf("\x1b[2KEvent      : %.32s\n", view->last_event_name);
    printf("\x1b[2KTrace      : %.24s -> %.24s\n",
           view->before_state_name, view->after_state_name);
    printf("\x1b[2KResult     : %.32s\n", view->result_name);
    printf("\x1b[2K\n\x1b[2KCommand menu  > = cursor\n");

    for (size_t row = 0U; row < visible_rows; ++row)
    {
        printf("\x1b[2K");
        if (row < rows)
        {
            const size_t index = first_visible + row;
            const bool selected = (index == ui->selected_index);
            printf("%s %c %.32s\x1b[0m",
                   selected ? "\x1b[1;36m" : "",
                   selected ? '>' : ' ',
                   ui->commands[index].name);
        }
        putchar('\n');
    }

    printf("\x1b[2KMenu: %zu-%zu / %zu\n", first_visible + 1U,
           first_visible + rows, ui->command_count);
    printf("\x1b[2K----------------------------------------------------------\n");
    printf("\x1b[2KUp/Down or W/S : select  |  Enter : send command\n");
    printf("\x1b[2KR : reset FSM  |  Q / Esc / Ctrl+C : quit\x1b[J");

    if ((fflush(stdout) == EOF) || ferror(stdout))
    {
        return FSM_CLI_OUTPUT_ERROR;
    }
    return FSM_CLI_OK;
}

fsm_cli_result_t FsmCli_Close(fsm_cli_t *ui)
{
    fsm_cli_result_t result = FSM_CLI_OK;

    if (ui == NULL)
    {
        return FSM_CLI_INVALID_ARG;
    }
    if (!ui->initialized)
    {
        return FSM_CLI_NOT_READY;
    }
    if ((fputs("\x1b[0m\x1b[?25h\x1b[?1049l", stdout) == EOF) ||
        (fflush(stdout) == EOF))
    {
        result = FSM_CLI_OUTPUT_ERROR;
    }
    // Attempt every restoration even if an earlier operation failed.
    if (!SetConsoleCursorInfo(ui->output_handle, &ui->saved_cursor))
    {
        result = FSM_CLI_CONSOLE_ERROR;
    }
    if (!SetConsoleMode(ui->input_handle, ui->saved_input_mode))
    {
        result = FSM_CLI_CONSOLE_ERROR;
    }
    if (!SetConsoleMode(ui->output_handle, ui->saved_output_mode))
    {
        result = FSM_CLI_CONSOLE_ERROR;
    }
    ui->initialized = false;
    return result;
}
