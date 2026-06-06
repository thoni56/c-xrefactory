enum E {
  E0, E1, E2, E3
};

struct X {
  int a;
};

struct Y {
  int a;
  struct X b;
  struct X c[2];
};

struct Y s = { /* c-xref is able to parse such an initializer from now */
  .a = E0,
  .b.a = E1,
  .c[E0].a = E0,
  .c[E1].a = E1,
};

int a [2][2] = {
  [E0][E0] = E0,
  [E0][E1] = E1,
  [E1][E0] = E2,
  [E1][E1] = E3
};

int main()
{
  return 0;
}
