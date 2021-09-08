#include "yco.h"
#include "yco_ctx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct YCoTask tasks[MAX_CO_COUNT];
static int cur_ind;
static void *cur_reg_area;

void _yco_clear_task_areas(struct YCoTask *task) {
    // 8 reg area  + 3 func area
    memset(&(task->r15), sizeof(void *) * (8+3), 0x0);
    return ;
}

int yco_init() {
    memset(tasks, MAX_CO_COUNT*sizeof(struct YCoTask), 0x0);

    // main
    tasks[0].state = YCO_STATE_RUNNING;

    for (int i=1; i!=MAX_CO_COUNT; i++) {
        tasks[i].state = YCO_STATE_AVAILABLE;
    }

    yco_set_cur_task(0);
    return 0;
}

int yco_cleanup() {
    for (int i=0; i!=MAX_CO_COUNT; i++) {
        if (tasks[i].mem != NULL) {
            free(tasks[i].mem);
            tasks[i].mem = NULL;
        }
    }

    yco_set_cur_task(0);
    memset(tasks, MAX_CO_COUNT*sizeof(struct YCoTask), 0x0);

    return 0;
}

int yco_get_cur_ind() {
    return cur_ind;
}

int yco_set_cur_task(int ind) {
    if (ind < 0 || ind >= MAX_CO_COUNT) {
        return -1;
    }

    cur_ind = ind;
    cur_reg_area = &(tasks[ind].r15);
    return 0;
}

int yco_get_next_available_ind() {
    for (int i=1; i!=MAX_CO_COUNT; i++) {
        if (tasks[i].state == YCO_STATE_AVAILABLE) {
            return i;
        }
    }

    return -1;
}

int yco_get_next_schedulable_ind() {
    for (int i=0; i!=MAX_CO_COUNT; i++) {
        int next_ind = i + cur_ind + 1;
        if (next_ind >= MAX_CO_COUNT) {
            next_ind -= MAX_CO_COUNT;
        }

        if (tasks[next_ind].state == YCO_STATE_CREATED ||
                tasks[next_ind].state == YCO_STATE_RUNNING ) {
            return next_ind;
        }
    }

    return -1;
}

void yco_create(int *yco_id, void *(*f)(void *), void *input, void *output, int join) {
    yco_push_ctx(cur_reg_area); // !!! always put this at the beginning

    int next_ind = yco_get_next_available_ind();
    if (next_ind == -1) {
        fprintf(stderr, "create tasks failed, pool full");
        exit(-1);
    }

    if (yco_id != NULL) {
        *yco_id = next_ind;
    }

    struct YCoTask *task = tasks + next_ind;
    _yco_clear_task_areas(task);

    if (task->mem == NULL) {
        task->mem = malloc(DEFAULT_STACK_SIZE);
        if (task->mem == NULL) {
            fprintf(stderr, "malloc task stack failed");
            exit(-1);
        }
    }
    memset(task->mem, DEFAULT_STACK_SIZE, 0x0);
    task->stack_start = task->mem + DEFAULT_STACK_SIZE - RESERVED_STACK_SIZE;

    if (join != 0) {
        task->attr |= YCO_ATTR_JOINABLE;
    }

    task->rsp = (uint64_t)(task->stack_start - sizeof(void *));
    task->rip = (uint64_t)&yco_wrapper;

    task->f = f;
    task->input = input;
    task->output = output;

    // go run
    yco_set_cur_task(next_ind);
    task->state = YCO_STATE_RUNNING;
    yco_use_ctx_run(&(task->r15), &(task->f));
}

void yco_wrapper(void *(*f)(void *), void *input, void *output) {
    uint64_t res = (uint64_t)f(input);
    if (output != NULL) {
        *((uint64_t *)output) = res;
    }

    struct YCoTask *task = tasks + cur_ind;
    if ((task->attr & YCO_ATTR_JOINABLE) == 0) {
        task->state = YCO_STATE_AVAILABLE;
        task->attr = 0;
        _yco_clear_task_areas(task);
    } else {
        task->state = YCO_STATE_EXITED;
    }

    int next_ind = yco_get_next_schedulable_ind();
    if (next_ind == -1) {
        fprintf(stderr, "one task return but no more to schedule");
        exit(-1);
    }

    yco_set_cur_task(next_ind);
    struct YCoTask *next_task = tasks + next_ind;
    if (next_task->state != YCO_STATE_RUNNING) {
        next_task->state = YCO_STATE_RUNNING;
    }
    yco_use_ctx(&(next_task->r15));
}

void yco_schedule() {
    yco_push_ctx(cur_reg_area); // !!! always put this at the beginning

    int next_ind = yco_get_next_schedulable_ind();
    if (next_ind == -1) {
        return;
    }
    // printf("cur ind: %d, next ind: %d\n", cur_ind, next_ind);

    yco_set_cur_task(next_ind);
    struct YCoTask *next_task = tasks + next_ind;
    if (next_task->state != YCO_STATE_RUNNING) {
        next_task->state = YCO_STATE_RUNNING;
    }
    yco_use_ctx(&(next_task->r15));
}

void yco_try_schedule_to(int next_ind) {
    yco_push_ctx(cur_reg_area); // !!! always put this at the beginning

    if (next_ind < 0 || next_ind > MAX_CO_COUNT) {
        return ;
    }

    struct YCoTask *next_task = tasks + next_ind;
    if (next_task->state != YCO_STATE_RUNNING &&
            next_task->state != YCO_STATE_RUNNING) {
        return ;
    }

    yco_set_cur_task(next_ind);
    if (next_task->state != YCO_STATE_RUNNING) {
        next_task->state = YCO_STATE_RUNNING;
    }
    yco_use_ctx(&(next_task->r15));
}

int yco_join(int ind) {
    struct YCoTask *task = tasks + ind;
    if ((task->attr & YCO_ATTR_JOINABLE) == 0) {
        return -1;
    }

    if (task->state != YCO_STATE_RUNNING &&
            task->state != YCO_STATE_RUNNING &&
            task->state != YCO_STATE_EXITED) {
        return -1;
    }

    while (task->state != YCO_STATE_EXITED) {
        yco_schedule();
    }

    task->state = YCO_STATE_AVAILABLE;
    task->attr = 0;
    _yco_clear_task_areas(task);
    return 0;
}
