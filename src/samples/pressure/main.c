#include "yco.h"

#include <stdio.h>
#include <string.h>

void *func(void *input) {
    uint64_t res = (uint64_t)input;
    for (int i=0; i!=10; i++) {
        res ++;
        yco_schedule();
    }
    return (void *)res;
}

int main() {
    printf("step into main\n");
    yco_init();

    struct YCoAttr attr;
    yco_attr_init(&attr, 1, 1024*512);

    yco_id_t coid;
    uint64_t ress[100000];
    memset(ress, sizeof(uint64_t)*100000, 0x0);
    for (int i=0; i!=100000; i++) {
        yco_id_t coid5;
        yco_create(&coid, &func, (void *)2, &ress[i], &attr);
    }

    yco_wait_all();

    uint64_t total = 0;
    for (int i=0; i!=100000; i++) {
        total += ress[i];
    }
    printf("total: %lu\n", total);

    yco_cleanup();
    printf("step out of main\n");

    return 0;
}
