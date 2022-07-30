#include "user/user.h"

int main() {
  int memo[34];
  int cnt = 0;
  for (int i = 2; i <= 35; ++i)
    memo[cnt++] = i;
  while (1) {
    int pid;
    int prime_pip[2];
    pipe(prime_pip);
    pid = fork();
    if (pid == 0) {   // child
      close(prime_pip[1]);
      int n;
      cnt = 0;
      if (read(prime_pip[0], &n, sizeof(n)) != 0) {
        memo[cnt++] = n;
        while (read(prime_pip[0], &n, sizeof(n)) != 0)
          memo[cnt++] = n;
        close(prime_pip[0]);
      } else {
        close(prime_pip[0]);
        break;
      }
    } else {  // parent, read from left, send to right
      close(prime_pip[0]);
      if (cnt > 0) {
        int base = memo[0];
        printf("prime %d\n", base);
        for (int i = 1; i < cnt; ++i) {
          if (memo[i] % base) {
            write(prime_pip[1], memo + i, sizeof(int));
          }
        }
      }
      close(prime_pip[1]);
      int status;
      wait(&status);
      break;
    }
  }
  exit(0);
}
