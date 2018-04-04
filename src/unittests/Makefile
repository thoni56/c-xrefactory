CFLAGS = -g -MMD -MT .d -fPIC -I..

all: symtab_tests.so
	cgreen-runner $<

symtab_tests.so: symtab_tests.o symtab.o
	gcc -shared -o $@ $^ -lcgreen

symtab.o: ../symtab.c

%.o: %.c
	gcc -o $@ $(CFLAGS) -c $<

%.o: ../%.c
	gcc -o $@ $(CFLAGS) -c $<



-include .d/*
