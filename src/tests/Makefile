# Unit tests for c-xrefactory units using cgreen (github.com/cgreen-devs/cgreen)

vpath %.c ..

CFLAGS = -fPIC -g -MMD

all: filetab.so
	cgreen-runner $^

filetab.so: filetab_tests.o filetab.o misc_mock.o commons_mock.o
	gcc -shared -o $@ $^

clean:
	rm *.o *.so

-include *.d
