
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

// ==================================================
// 1. HSM definitions
// ==================================================

typedef enum {
    ST_NONE = -1,
    ST_TOP,
    ST_OFF,
    ST_ON,       // Parent state
    ST_SOLID,    // Child of ON
    ST_BLINK,     // Child of ON
    ST_DIM
} state_t;

typedef enum {
    EV_SHORT,
    EV_LONG,
    EV_TIMEOUT,
    EV_RESET
} event_t;

typedef enum {
    H_SUPER,      // Parent에게 이벤트 위임
    H_HANDLED,    // 현재 상태에서 처리 완료
    H_REJECTED,   // Guard 거절
    H_TRAN        // 상태 전이 요청
} handler_result_t;

typedef struct {
    state_t state;       // 현재 활성 Leaf State

    bool led_on;
    bool on_service;
    bool allow_start;    // Guard 실습용
    bool allow_blink;

    bool timer_running;
    uint16_t timer_ms;
} led_hsm_t;

// ==================================================
// 2. State hierarchy
// ==================================================

static const char *StateName(state_t s)
{
    switch (s) {
    case ST_TOP:   return "TOP";
    case ST_OFF:   return "OFF";
    case ST_ON:    return "ON";
    case ST_SOLID: return "SOLID";
    case ST_BLINK: return "BLINK";
    case ST_DIM:   return "DIM";
    default:       return "NONE";
    }
}

static state_t Parent(state_t s)
{
    switch (s) {
    case ST_SOLID:
    case ST_BLINK:
    case ST_DIM:
        return ST_ON;

    case ST_ON:
    case ST_OFF:
        return ST_TOP;

    default:
        return ST_NONE;
    }
}


// ==================================================
// 3. Entry / Exit Actions
// ==================================================

static void Led_Write(led_hsm_t *h, bool on)
{
    if (h->led_on != on) {
        h->led_on = on;
        printf("  LED = %s\n", on ? "ON" : "OFF");
    }
}

static void OnEntry(led_hsm_t *h, state_t s)
{
    printf("  ENTRY %s\n", StateName(s));

    switch (s) {
    case ST_OFF:
        Led_Write(h, false);
        break;

    case ST_ON:
        h->on_service = true;
        break;

    case ST_SOLID:
        Led_Write(h, true);
        break;

    case ST_BLINK:
        h->timer_ms = 0;
        h->timer_running = true;
        Led_Write(h, true);
        break;

    default:
        break;
    }
}

static void OnExit(led_hsm_t *h, state_t s)
{
    printf("  EXIT  %s\n", StateName(s));

    switch (s) {
    case ST_BLINK:
        h->timer_running = false;
        h->timer_ms = 0;
        break;

    case ST_ON:
        h->on_service = false;
        Led_Write(h, false);
        break;

    default:
        break;
    }
}

// ==================================================
// 4. State handlers: 핵심 학습 구간
// ==================================================

static handler_result_t HandleState(
    led_hsm_t *h,
    state_t s,
    event_t event,
    state_t *target)
{
    switch (s) {

    case ST_OFF:
        if (event == EV_SHORT) {
            // Guard
            if (!h->allow_start) {
                return H_REJECTED;
            }

            *target = ST_SOLID;
            return H_TRAN;
        }
        return H_SUPER;

    case ST_SOLID:
        if (event == EV_SHORT) {

            if(!h->allow_blink)
            {
                return H_REJECTED;   
            }


            *target = ST_BLINK;
            return H_TRAN;
        }
        return H_SUPER;

    case ST_BLINK:
        if (event == EV_SHORT) {
            *target = ST_DIM;
            return H_TRAN;
        }

        if (event == EV_TIMEOUT) {
            Led_Write(h, !h->led_on);
            return H_HANDLED;
        }
        return H_SUPER;
    case ST_DIM:
        // do noting
        if (event == EV_TIMEOUT) {
            Led_Write(h, !h->led_on);
            return H_HANDLED;
        }
        return H_SUPER;

    case ST_ON:
        // SOLID와 BLINK가 공유하는 이벤트
        if (event == EV_LONG) {
            *target = ST_OFF;
            return H_TRAN;
        }
        return H_SUPER;

    case ST_TOP:
        if (event == EV_RESET) {
            if (h->state == ST_OFF) {
                return H_HANDLED;
            }

            *target = ST_OFF;
            return H_TRAN;
        }
        return H_SUPER;

    default:
        return H_SUPER;
    }
}

// ==================================================
// 5. HSM Engine: common ancestor + entry/exit
// ==================================================

static bool IsAncestor(state_t ancestor, state_t s)
{
    while (s != ST_NONE) {
        if (s == ancestor) {
            return true;
        }
        s = Parent(s);
    }
    return false;
}

static state_t FindLca(state_t a, state_t b)
{
    while (a != ST_NONE) {
        if (IsAncestor(a, b)) {
            return a;
        }
        a = Parent(a);
    }
    return ST_NONE;
}

static void Hsm_Transition(led_hsm_t *h, state_t target)
{
    state_t lca = FindLca(h->state, target);
    state_t s = h->state;

    assert(lca != ST_NONE);

    // Exit: Leaf -> common ancestor
    while (s != lca) {
        OnExit(h, s);
        s = Parent(s);
    }

    // Entry path를 역순으로 저장
    state_t path[4];
    int depth = 0;

    for (s = target; s != lca; s = Parent(s)) {
        assert(depth < 4);
        path[depth++] = s;
    }

    // Entry: common ancestor -> Target
    while (depth > 0) {
        OnEntry(h, path[--depth]);
    }

    h->state = target;
}

// ==================================================
// 6. Hierarchical event dispatch
// ==================================================

static void Hsm_Dispatch(led_hsm_t *h, event_t event)
{
    state_t s = h->state;

    printf("\nEVENT %d at %s\n",
           (int)event, StateName(s));

    while (s != ST_NONE) {
        state_t target = ST_NONE;

        handler_result_t r =
            HandleState(h, s, event, &target);

        if (r == H_SUPER) {
            state_t p = Parent(s);

            if (p != ST_NONE) {
                printf("  %s -> SUPER(%s)\n",
                       StateName(s), StateName(p));
            }

            s = p;
            continue;
        }

        if (r == H_REJECTED) {
            printf("  GUARD REJECTED by %s\n",
                   StateName(s));
            return;
        }

        if (r == H_HANDLED) {
            printf("  HANDLED by %s\n", StateName(s));
            return;
        }

        if (r == H_TRAN) {
            printf("  TRANSITION by %s -> %s\n",
                   StateName(s), StateName(target));

            Hsm_Transition(h, target);
            return;
        }
    }

    printf("  UNHANDLED EVENT\n");
}

// ==================================================
// 7. Initialization and periodic timer source
// ==================================================

static void Hsm_Init(led_hsm_t *h)
{
    *h = (led_hsm_t){0};

    h->state = ST_TOP;
    h->allow_start = true;
    h->allow_blink = true;

    OnEntry(h, ST_TOP);
    Hsm_Transition(h, ST_OFF);
}

static void App_Run1ms(led_hsm_t *h)
{
    if (!h->timer_running) {
        return;
    }

    ++h->timer_ms;

    if (h->timer_ms >= 500U) {
        h->timer_ms = 0;
        Hsm_Dispatch(h, EV_TIMEOUT);
    }
}

static void PrintStatus(const led_hsm_t *h)
{
    printf("\nSTATE=%s  LED=%s  TIMER=%s  GUARD=%s\n",
           StateName(h->state),
           h->led_on ? "ON" : "OFF",
           h->timer_running ? "RUN" : "STOP",
           h->allow_start ? "ALLOW" : "BLOCK");
}

// ==================================================
// 8. PC virtual button and scheduler
// ==================================================

int main(void)
{
    led_hsm_t hsm;
    char input[32];

    Hsm_Init(&hsm);

    puts("\ns: SHORT button");
    puts("l: LONG button");
    puts("t: advance virtual time by 500ms");
    puts("g: toggle start guard");
    puts("r: RESET");
    puts("q: quit");

    PrintStatus(&hsm);

    while (true) {
        printf("\ncmd> ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        switch (input[0]) {
        case 's':
            Hsm_Dispatch(&hsm, EV_SHORT);
            break;

        case 'l':
            Hsm_Dispatch(&hsm, EV_LONG);
            break;

        case 't':
            for (int i = 0; i < 500; ++i) {
                App_Run1ms(&hsm);
            }
            break;

        case 'g':
            hsm.allow_start = !hsm.allow_start;
            break;

        case 'r':
            Hsm_Dispatch(&hsm, EV_RESET);
            break;

        case 'q':
            return 0;

        default:
            puts("Unknown command");
            break;
        }

        PrintStatus(&hsm);
    }

    return 0;
}
