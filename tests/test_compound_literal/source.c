typedef struct s {
    int integer;
    char *string;
} S;

S s;
S *sp;

int *p = (int[]){1, 2, 3}; /* Array compound literal */

struct Flex {                   /* Empty struct */
    int length;
    char data[];  // No fixed size
};

int main() {
    sp->integer = 1;
    sp->string = "Hello,";

    s.integer = 2;
    s.string = "brave, new,";

    s = (S){.integer = 3, .string = "world!"};
}
