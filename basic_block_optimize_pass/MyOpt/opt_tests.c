// strength reduction: 2*c -> shl c, 1    f*8 -> shl f, 3
int test_strength() {
  int a = 3, b = 4, c, d, e, f;
  d = 2 * c;
  e = f * 8;
  return a;
}

// a/a = 1, b/b = 1, b-b = 0, then the rest folds to 23
int compute(int a, int b) {
  int result = (a / a);

  result *= (b / b);
  result += (b - b);
  result /= result;
  result -= result;
  result += 23;
  return result;
}

// c = d, so e = c + b becomes e = d + 4
int test_copy() {
  int b = 4, c, d, e;
  c = d;
  e = c + b;
  return e;
}

// 4+2+3 = 9, 0+2+3 = 5, 5*9 = 45, 45/2 = 22
int compute_fold() {
  int result = 0;
  int a = 2;
  int b = 3;
  int c = 4 + a + b;

  result += a;
  result += b;
  result *= c;
  result /= 2;
  a = result;
  return result;
}

// second a = 3 and b = 4 are redundant
int test_redundant() {
  int a = 3, b = 4, c;
  a = 3;
  b = 4;
  c = a + b;
  return a;
}
