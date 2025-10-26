#define delete_comma(...) int a, ##__VA_ARGS__;

void function() {
    int delete_comma();
}
