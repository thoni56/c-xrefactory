#define paste_empty(x, ...) x ##__VA_ARGS__;
int paste_empty(a);

#define PASTE(a, b) a##b
#define EMPTY
int PASTE(EMPTY, foo); // Left operand is empty
int PASTE(foo, EMPTY); // Right operand is empty
int PASTE(, foo);       /* Actually illegal... */
