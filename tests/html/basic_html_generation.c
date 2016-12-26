#include "basic_html_generation.h"

static int counter;

int main(int argc, char **argv) {
    Options options;
    options.lines = counter++;
    options.columns = counter++;
}
