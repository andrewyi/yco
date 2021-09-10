#include "yco.h"

#include <stdio.h>
#include <string.h>

void *func(void *input) {
    yco_schedule();
    return func(input);
}

int main() {
    printf("step into main\n");
    yco_init();

    struct YCoAttr attr;
    yco_attr_init(&attr, 1, 1024*512);

    // try stack overflow
    yco_id_t coid;
    yco_create(&coid, &func, NULL, NULL, &attr);
    yco_join(coid);

    yco_cleanup();
    printf("step out of main\n");

    return 0;
}
