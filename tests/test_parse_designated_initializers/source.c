struct Point { int x, y; };
struct Point p = { .y = 2, .x = 1 };  // Fields initialized out of order
struct Container {
    struct Point point;
};
struct Container c = { .point = { .y = 10, .x = 20 } };
char ascii_table[128] = {['A'] = 65, ['Z'] = 90};
int i;
