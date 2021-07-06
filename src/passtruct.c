typedef struct { int a; int b; } Struct;

void f(Struct s) {
}

void g(void) {
    f((Struct){0,0});
}
