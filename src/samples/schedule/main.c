#include "yco.h"

#include <stdio.h>
#include <string.h>

void *func1(void *input) {
    uint64_t total = (uint64_t)input;
    for (int i=0; i!=10; i++) {
        total += i;
        yco_schedule();
    }
    return (void *)total;
}

void *func2(void *input) {
    uint64_t total = (uint64_t)input;
    for (int i=10; i!=20; i++) {
        total += i;
        yco_schedule();
    }
    return (void *)total;
}

void *func3(void *input) {
    uint64_t total = (uint64_t)input;
    for (int i=20; i!=30; i++) {
        total += i;
        yco_schedule();
    }
    return (void *)total;
}

int main() {
    printf("step into main\n");
    yco_init();

    struct YCoAttr attr;
    yco_attr_init(&attr, 1, 1024*512);

    yco_id_t coid1, coid2, coid3;
    uint64_t res1, res2, res3;

    yco_create(&coid1, &func1, (void *)100000, (void *)&res1, &attr);
    printf("coroutine1 created, id: %p, num id: %ld\n", coid1, get_task_num_id(coid1));
    yco_join(coid1);
    printf("res1: %lu\n", res1);

    yco_create(&coid2, &func2, (void *)10000, (void *)&res2, &attr);
    yco_create(&coid3, &func3, (void *)1000, (void *)&res3, &attr);

    printf("coroutine2 created, id: %p, num id: %ld\n", coid2, get_task_num_id(coid2));
    printf("coroutine3 created, id: %p, num id: %ld\n", coid3, get_task_num_id(coid3));

    yco_join(coid2);
    yco_join(coid3);

    printf("res2: %lu\n", res2);
    printf("res2: %lu\n", res3);

    yco_cleanup();
    printf("step out of main\n");

    return 0;
}
