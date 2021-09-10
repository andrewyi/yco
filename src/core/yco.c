#include "yco.h"
#include "yco_ctx.h"

#include <stdlib.h>
#include <string.h>

/*
 * internal functions
 * */

static void _yco_clear_tasks(struct YCoTask *header) {
    struct YCoTask *next = NULL;
    while (header != NULL) {
        if (header->mem != NULL) {
            free(header->mem);
        }
        next = header->next;
        free(header);
        header = next;
    }
}

// task must not be NULL
static void _yco_empty_task_except_mem(struct YCoTask *task) {
    task->prev = NULL;
    task->next= NULL;
    task->num_id = 0;
    task->state = 0;
    task->attr = 0;
    // task->stack_size no touched
    // task->mem no touched
    // task->stack_start no touched
    task->r15 = 0;
    task->r14 = 0;
    task->r13 = 0;
    task->r12 = 0;
    task->rbx = 0;
    task->rbp = 0;
    task->rsp = 0;
    task->rip = 0;
    task->f = NULL;
    task->input = NULL;
    task->output= NULL;
}

// task must not be NULL
// return 0 on sucess, -1 failed
static int _yco_resize_task_stack(struct YCoTask *task, uint32_t size) {
    if (task->stack_size == size) {
        memset(task->mem, size, 0x0);
        return 0;
    }

    if (task->mem != NULL) {
        free(task->mem);
    }

    task->mem = malloc(size);
    if (task->mem == NULL) {
        return -1;
    }
    memset(task->mem, size, 0x0);
    task->stack_size = size;
    task->stack_start = task->mem + size - RESERVED_STACK_HEADER_SIZE;

    return 0;
}

// task must not be NULL
static void _yco_clear_task_areas(struct YCoTask *task) {
    // 8 reg area  + 3 func area
    // memset(&(task->r15), sizeof(void *) * (8+3), 0x0);
    task->r15 = 0;
    task->r14 = 0;
    task->r13 = 0;
    task->r12 = 0;
    task->rbx = 0;
    task->rbp = 0;
    task->rsp = 0;
    task->rip = 0;
    task->f = NULL;
    task->input = NULL;
    task->output= NULL;
}

// task must not be NULL, will raise SIGSEGV if overflow
static void _yco_check_stackoverflow(struct YCoTask *task) {
    // do not check main coroutine, leave it to os
    if ((task->attr & YCO_ATTR_MAIN) != 0) {
        return ;
    }

    uint32_t used_stack_size = (uint32_t)(
            (uint64_t)task->stack_start - task->rsp);
    uint32_t max_stack_size = task->stack_size -
                              RESERVED_STACK_HEADER_SIZE -
                              RESERVED_STACK_TAIL_SIZE;

    if (used_stack_size > max_stack_size) {
        // stack overflow
        // printf("%lu: %u - %u\n", task->num_id, used_stack_size, max_stack_size);
        raise(SIGSEGV);
    }
}

/*
 * global vars
 * */

static struct YCoTask *running_tasks; // created/running state
static struct YCoTask *exited_tasks;
static struct YCoTask *reclaimed_tasks;
static struct YCoTask *cur_task;
static struct YCoAttr default_attr;

static struct sigaction orig_sigact;

/*
 * facility functions
 * */

int64_t get_next_task_num_id() {
    static int64_t id = 1;
    if (id <= 0) { // 0 reserved for main task
        id = 1;
    }
    return id ++;
}

// return -1 when yco_id is NULL
int64_t get_task_num_id(yco_id_t yco_id) {
    if (yco_id == NULL) {
        return -1;
    }

    struct YCoTask *task = (struct YCoTask *)yco_id;
    return (int64_t)task->num_id;
}

// return -1 when failed
int yco_init() {
    // set cur_task as main task
    cur_task = (struct YCoTask *)malloc(sizeof(struct YCoTask));
    if (cur_task == NULL) {
        return -1;
    }
    cur_task->state = YCO_STATE_RUNNING;
    cur_task->attr |= YCO_ATTR_MAIN;

    running_tasks = cur_task;
    exited_tasks = NULL;
    reclaimed_tasks = NULL;

    default_attr.is_joinable = 0;
    default_attr.stack_size = DEFAULT_STACK_SIZE;

    struct sigaction in;
    in.sa_sigaction = preempt_sig_handler;
    in.sa_flags |= SA_SIGINFO;
    if (sigaction(PREEMPT_SIGNAL, &in, &orig_sigact) != 0) {
        return -1;
    }

    return 0;
}

// always return 0
int yco_cleanup() {
    _yco_clear_tasks(running_tasks);
    _yco_clear_tasks(exited_tasks);
    _yco_clear_tasks(reclaimed_tasks);
    cur_task = NULL;

    sigaction(PREEMPT_SIGNAL, &orig_sigact, NULL);
    return 0;
}

// return NULL if failed
struct YCoTask *yco_make_task_struct() {
    struct YCoTask *task = NULL;

    if (reclaimed_tasks == NULL) {
        task = (struct YCoTask *)malloc(sizeof(struct YCoTask));

    } else {
        task = reclaimed_tasks;
        reclaimed_tasks = reclaimed_tasks->next;
        if (reclaimed_tasks != NULL) {
            reclaimed_tasks->prev = NULL;
        }
    }
    _yco_empty_task_except_mem(task);

    return task;
}

// attr must not be null
int yco_attr_init(
        struct YCoAttr *attr,
        int is_joinable,
        uint32_t stack_size
        )
{
    if (attr == NULL) {
        return -1;
    }
    attr->is_joinable = is_joinable;
    attr->stack_size = stack_size;
    return 0;
}

void yco_create(
        yco_id_t *yco_id,
        void *(*f)(void *),
        void *input,
        void *output,
        struct YCoAttr *attr
        )
{
    yco_push_ctx(&(cur_task->r15)); // !!! always put this at the beginning

    if (attr == NULL) {
        attr = &default_attr;
    }

    struct YCoTask *new_task = yco_make_task_struct();
    if (new_task == NULL) {
        if (yco_id != NULL) {
            *(uint64_t *)yco_id = INVALID_YCO_ID;
        }
        return ;
    }
    if (yco_id != NULL) {
        *yco_id = (yco_id_t)new_task;
    }
    _yco_empty_task_except_mem(new_task);
    _yco_resize_task_stack(new_task, attr->stack_size);

    new_task->num_id = get_next_task_num_id();

    if (attr->is_joinable != 0) {
        new_task->attr |= YCO_ATTR_JOINABLE;
    }

    new_task->rsp = (uint64_t)(new_task->stack_start - sizeof(void *));
    new_task->rip = (uint64_t)&yco_wrapper;
    new_task->f = f;
    new_task->input = input;
    new_task->output = output;

    struct YCoTask *tmp = cur_task->next;
    cur_task->next = new_task;
    new_task->prev = cur_task;
    if (tmp != NULL) {
        new_task->next = tmp;
        tmp->prev = new_task;
    }

    cur_task = new_task;
    cur_task->state = YCO_STATE_RUNNING;
    yco_use_ctx_run(&(cur_task->r15), &(cur_task->f));
}

void yco_wrapper(void *(*f)(void *), void *input, void *output) {
    uint64_t res = (uint64_t)f(input);
    if (output != NULL) {
        *((uint64_t *)output) = res;
    }

    struct YCoTask *prev_task = cur_task->prev; // would never be null
    struct YCoTask *next_task = cur_task->next;
    if (next_task == NULL) {
        prev_task->next = NULL;

    } else {
        prev_task->next = next_task;
        next_task->prev = prev_task;
    }

    cur_task->prev = NULL;
    cur_task->next = NULL;

    if ( (cur_task->attr & YCO_ATTR_JOINABLE) != 0 ) {
        cur_task->state = YCO_STATE_EXITED;

        cur_task->next = exited_tasks;
        if (exited_tasks != NULL) {
            exited_tasks->prev = cur_task;
        }
        exited_tasks = cur_task;

    } else {
        cur_task->state = YCO_STATE_UNDEFINED;

        cur_task->next = reclaimed_tasks;
        if (reclaimed_tasks != NULL) {
            reclaimed_tasks->prev = cur_task;
        }
        reclaimed_tasks = cur_task;
    }

    if (next_task == NULL) {
        next_task = running_tasks;
    }
    cur_task = next_task;
    yco_use_ctx(&(cur_task->r15));
}

void yco_schedule() {
    yco_push_ctx(&(cur_task->r15)); // !!! always put this at the beginning

    _yco_check_stackoverflow(cur_task);

    struct YCoTask *next_task = cur_task->next;
    if (next_task == NULL) {
        if ((cur_task->attr & YCO_ATTR_MAIN) != 0) {
            // only main coroutine exists
            return ; 
        }

        // wrap to header
        next_task = running_tasks;
    }

    cur_task = next_task;
    yco_use_ctx(&(cur_task->r15));
}

int yco_join(yco_id_t yco_id) {
    struct YCoTask *task = (struct YCoTask *)yco_id;
    if ((task->attr & YCO_ATTR_JOINABLE) == 0) {
        return -1;
    }

    if (task->state != YCO_STATE_CREATED &&
            task->state != YCO_STATE_RUNNING &&
            task->state != YCO_STATE_EXITED) {
        return -1;
    }

    while (task->state != YCO_STATE_EXITED) {
        yco_schedule();
    }

    struct YCoTask *prev_task = task->prev;
    struct YCoTask *next_task = task->next;
    if (task == exited_tasks) { // head
        exited_tasks = exited_tasks->next;
        if (exited_tasks != NULL) {
            exited_tasks->prev = NULL;
        }

    } else {
        if (next_task == NULL) {
            prev_task->next = NULL;

        } else {
            prev_task->next = next_task;
            next_task->prev = prev_task;
        }
    }

    task->prev = NULL;
    task->next = NULL;

    task->state = YCO_STATE_UNDEFINED;

    task->next = reclaimed_tasks;
    if (reclaimed_tasks != NULL) {
        reclaimed_tasks->prev = task;
    }
    reclaimed_tasks = task;

    return 0;
}

void yco_wait_all() {
    while (running_tasks->next != NULL) {
        yco_schedule();
    }

    while (exited_tasks != NULL) {
        yco_join((yco_id_t)exited_tasks);
    }
}

void preempt_stub() {
    asm volatile (
            "push %rax \n\t"
            "push %rbx \n\t"
            "push %rcx \n\t"
            "push %rdx \n\t"
            // "push %rsp \n\t"
            // "push %rbp \n\t"
            "push %rsi \n\t"
            "push %rdi \n\t"
            "push %r8 \n\t"
            "push %r9 \n\t"
            "push %r10 \n\t"
            "push %r11 \n\t"
            "push %r12 \n\t"
            "push %r13 \n\t"
            "push %r14 \n\t"
            "push %r15 \n\t"
            "pushfq \n\t"
            );

    yco_schedule();

    asm volatile (
            "popfq \n\t"
            "pop %r15 \n\t"
            "pop %r14 \n\t"
            "pop %r13 \n\t"
            "pop %r12 \n\t"
            "pop %r11 \n\t"
            "pop %r10 \n\t"
            "pop %r9 \n\t"
            "pop %r8 \n\t"
            "pop %rdi \n\t"
            "pop %rsi \n\t"
            // "pop %rbp \n\t"
            // "pop %rsp \n\t"
            "pop %rdx \n\t"
            "pop %rcx \n\t"
            "pop %rbx \n\t"
            "pop %rax \n\t"
            );
}

void preempt_sig_handler(int sig_num, siginfo_t *info, void *u) {
    ucontext_t *ctx = (ucontext_t *)u;
    uint64_t rip = (uint64_t)ctx->uc_mcontext.gregs[REG_RIP];
    uint64_t rsp = (uint64_t)ctx->uc_mcontext.gregs[REG_RSP];

    uint64_t new_rsp = rsp -0x8; // move rsp forward to hold return addr
    *(uint64_t *)new_rsp  = rip; // prepare return addr
    ctx->uc_mcontext.gregs[REG_RSP] = (long long int)(new_rsp);
    ctx->uc_mcontext.gregs[REG_RIP] = (long long int)&preempt_stub; // call
}
