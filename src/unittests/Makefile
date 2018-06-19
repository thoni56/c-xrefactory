CFLAGS = -g -MMD -MT .d -fPIC -I..

all: symtab_tests.so olcxtab_tests.so
	cgreen-runner $^

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
