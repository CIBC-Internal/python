#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void foo(void *p, void *q) { memmove(p, q, 19); }

int main() {
  char a[32] = "123456789000000000";
  foo(&a[9], a);
  if (strcmp(a, "123456789123456789000000000") != 0)
    return 1;
  foo(a, &a[9]);
  if (strcmp(a, "123456789000000000") != 0)
    return 1;

  return 0;
}
