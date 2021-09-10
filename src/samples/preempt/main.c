#include "yco.h"

#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/syscall.h>

void *func(void *input) {
    uint64_t flag = 0;

    while (1) {
        flag = (uint64_t)(flag + 1);
        if ((flag % 3000000000) == 0) {
            printf("in coroutine infinite loop\n");
        }
    }

    return (void *)1;
}


int main() {
    printf("step into main\n");
    printf("pid: %d\n", getpid());

    yco_init();
    printf("into main, use kill -SIGUSR2 %ld to force coroutine schedule\n",
            syscall(SYS_gettid));

    struct YCoAttr attr;
    yco_attr_init(&attr, 1, 1024*512);

    uint64_t res = 0;
    yco_id_t coid;
    yco_create(&coid, &func, (void *)NULL, (void *)&res, &attr);

    while (res == 0) {
        printf("in main, res still 0, go schedule\n");
        yco_schedule();
    }
    yco_join(coid);

    yco_cleanup();
    printf("step out of main\n");

    return 0;
}
