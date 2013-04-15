__attribute__((noinline)) int foo(int *p) {
  int r;
  asm ( "movl \$6, (%1)\n\t"
    "xorl %0, %0\n\t"
    : "=r" (r) : "r" (p) : "memory"
  );
  return r;
}

int main() {
  int p = 8;
  if ((foo(&p) ? : p) != 6)
    return 1;

  return 0;
}
