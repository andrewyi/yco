CC := gcc
CFLAGS := -ggdb -I./

all: yco.o yco_ctx.o

yco.o: yco.c yco.h yco_ctx.h
	$(CC) $(CFLAGS) -c $< -o $@

yco_ctx.o: yco_ctx.s yco_ctx.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f yco.o yco_ctx.o

.PHONY: all clean 
