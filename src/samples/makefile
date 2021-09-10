CC := gcc
CFLAGS := -ggdb -I../../core

all: yco main

yco:
	make -C ../../core

main: main.o ../../core/yco.o ../../core/yco_ctx.o
	$(CC) $(CFLAGS) $^ -o $@
	objdump -d main > main.s

main.o: main.c ../../core/yco.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f main.o main.s main
	make -C ../../core clean

.PHONY: all yco clean allclean
