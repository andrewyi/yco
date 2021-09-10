#ifndef _YCO_H_
#define _YCO_H_

#include <stdint.h>

#define __USE_GNU // to use register index
#include <ucontext.h>
#include <signal.h>

typedef void* yco_id_t; // the address of YCoTask

#define DEFAULT_STACK_SIZE (1024*1024*8)
#define RESERVED_STACK_HEADER_SIZE (1024*128)
#define RESERVED_STACK_TAIL_SIZE (1024*128)

#define INVALID_YCO_ID 0xFFFFFFFFFFFFFFFF

#define YCO_STATE_UNDEFINED 0 // never seen
#define YCO_STATE_CREATED 1 // never seen
#define YCO_STATE_RUNNING 2
#define YCO_STATE_SLEEPING 3 // waiting for events, not implemented now
#define YCO_STATE_EXITED 4

#define YCO_ATTR_MAIN (0x1<<0)
#define YCO_ATTR_JOINABLE (0x1<<1) // default detach

#define PREEMPT_SIGNAL SIGUSR2

struct YCoAttr {
    int is_joinable;
    uint32_t stack_size;
};

struct YCoTask {
    struct YCoTask *prev;
    struct YCoTask *next;

    int64_t num_id; // for debug purpose

    int state;
    uint64_t attr;
    uint32_t stack_size;
    void *mem;
    void *stack_start;

    // register area, &(task.r15)
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rip;

    // function area, &(task.f)
    void *(*f)(void *);
    void *input;
    void *output;
};

int64_t get_next_task_num_id();
int64_t get_task_num_id(yco_id_t);

int yco_init();
int yco_cleanup();

struct YCoTask *yco_make_task_struct();

int yco_attr_init(
        struct YCoAttr *attr,
        int is_joinable,
        uint32_t stack_size
        );
void yco_create(
        yco_id_t *,
        void *(*)(void *),
        void *,
        void *,
        struct YCoAttr *
        );
void yco_wrapper(void *(*)(void *), void *, void *);
void yco_schedule();
int yco_join(yco_id_t);
void yco_wait_all();

void preempt_stub();
void preempt_sig_handler(int , siginfo_t *, void *);

#endif //_YCO_H_
