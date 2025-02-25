#define CONCAT(a, b) a##b
int CONCAT(my, Var); // Should become "int myVar;"

#define TXT(a) b##a
int TXT(z) = 10; // Should become "int bz = 10;"

#define JOIN(a, b) a##b
JOIN(sta, tic) int j;  // Should become: "static int j;"
