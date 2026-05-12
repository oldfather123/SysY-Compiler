int main() {
  int a = 1;
  int b = 2;
  int c = 3;
  int d = 4;
  int e = 5;
  int f = 6;
  if (a * b + c < 6 && d != 0) {
    if (e || !a + 0) {
      if (c == 2 && d + e > 2) return 3;
      else {
        if (f % c && e) return 4;
        else {
          if (d / b + a >= 2) {
            if (e - f >= 0 || d > 4) return 6;
            else {
              if (c != f) {
                if (b + e * d > 10) {
                  if (!f) return 9;
                  else return 10;
                }
                else return 8;
              }
              else return 7;
            }
          }
          else return 5;
        }
      }
    }
    else return 2;
  }
  else return 1;
}