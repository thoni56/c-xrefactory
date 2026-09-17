typedef long number;

int int_value(int x) { return x; }
long number_value(number x) { return x; }
double default_value(double x) { return x; }

int selector;

double pick(void) {
    return _Generic(selector,
                    int: int_value,
                    number: number_value,
                    default: default_value)(selector);
}
