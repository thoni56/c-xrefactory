#define STRINGIFY_TOKEN(token) #token
#define expect(f) record(STRINGIFY_TOKEN(f))

void record(const char *name);

int helper(void) {
    return 1;
}

void test_helper(void) {
    expect(helper);
    helper();
}
