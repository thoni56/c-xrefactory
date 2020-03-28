typedef struct s {
    int integer;
    char *string;
} S;

S s;
S *sp;

sp->integer = 1;
sp->string = "Hello,";

s.integer = 2;
s.string = "brave, new,";
s = (S){.integer = 3, .string = "world!"};
