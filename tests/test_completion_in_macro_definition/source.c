struct point {
    int x;
    int y;
};

void use(struct point *p) {
#define FIELD(q) q->
    FIELD(p)
    ;
}
