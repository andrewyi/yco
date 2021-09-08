#ifndef _YCO_H_
#define _YCO_H_

#include "stdint.h"

#define MAX_CO_COUNT 100
#define DEFAULT_STACK_SIZE (1024*1024*8)
#define RESERVED_STACK_SIZE (1024*256)

#define YCO_STATE_AVAILABLE 0
#define YCO_STATE_CREATED 1
#define YCO_STATE_RUNNING 2
#define YCO_STATE_EXITED 3

#define YCO_ATTR_JOINABLE (0x1<<0) // default detach

struct YCoTask {
    int state;
    uint64_t attr;
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

void _yco_clear_task_areas(struct YCoTask *task);

int yco_init();
int yco_cleanup();

int yco_get_cur_ind();
int yco_set_cur_task(int);

int yco_get_next_available_ind();
int yco_get_next_schedulable_ind();

void yco_create(int *, void *(*)(void *), void *, void *, int);
void yco_wrapper(void *(*)(void *), void *, void *);
void yco_schedule();
void yco_try_schedule_to(int);
int yco_join(int);

#endif //_YCO_H_
