#define bool _Bool

int isxdigit(int c);

bool areHexDigits(char *s) {
    bool pass = 1;
    for (int i = 0; i < 8; i++)
        if (!isxdigit(s[i]))
            pass = 0;
    return pass;
}
