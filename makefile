CC := gcc
CFLAGS := -ggdb -I./src # -fno-stack-protector

main: main.o yco.o yco_ctx.o
	$(CC) $(CFLAGS) $^ -o main
	objdump -d main > main.s

main.o: main.c src/yco.h
	$(CC) $(CFLAGS) -c $< -o $@

yco.o: src/yco.c src/yco.h src/yco_ctx.h
	$(CC) $(CFLAGS) -c $< -o $@

yco_ctx.o: src/yco_ctx.s src/yco_ctx.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f main main.o yco.o yco_ctx.o main.s

.PHONY: clean
