#define inner_macro(...) #__VA_ARGS__

#define outer_macro(...) inner_macro(a, ##__VA_ARGS__ )

void function() {
        int a,b;
        outer_macro(a, b);
}
