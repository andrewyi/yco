#include "yco.h"

#include <stdio.h>

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

    int coid1, coid2, coid3;
    uint64_t res1, res2, res3;
    yco_create(&coid1, &func1, (void *)100000, (void *)&res1, 1);
    yco_create(&coid2, &func2, (void *)10000, (void *)&res2, 1);
    yco_create(&coid3, &func3, (void *)1000, (void *)&res3, 1);
    printf("coroutine1 created, id: %d\n", coid1);
    printf("coroutine2 created, id: %d\n", coid2);
    printf("coroutine3 created, id: %d\n", coid3);
    yco_join(coid1);
    yco_join(coid2);
    yco_join(coid3);
    printf("res1: %lu\n", res1);
    printf("res2: %lu\n", res2);
    printf("res2: %lu\n", res3);

    yco_cleanup();
    printf("step out of main\n");
    return 0;
}
