CFLAGS = -g -MMD -MT .d -fPIC -I..

UNITTESTS = $(patsubst %.c,%.so,$(wildcard *_tests.c))

all: $(UNITTESTS)
	cgreen-runner --suite unittests $^

symtab_tests.so: symtab_tests.o symtab.o
	gcc -shared -o $@ $^ -lcgreen

symtab.o: ../symtab.c

olcxtab_tests.so: olcxtab_tests.o olcxtab.o
	gcc -shared -o $@ $^ -lcgreen

olcxtab.o: ../olcxtab.c

%.o: %.c
	gcc -o $@ $(CFLAGS) -c $<

%.o: ../%.c
	gcc -o $@ $(CFLAGS) -c $<



-include .d/*
