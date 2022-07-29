#include "user/user.h"

int main() {
  int pip_p_to_c[2];
  int pip_c_to_p[2];
  if (pipe(pip_p_to_c) == -1 || pipe(pip_c_to_p) == -1) {
    printf("create pipe failed!\n");
    exit(1);
  }

  int pid = fork();
  char ch;
  if (pid == -1) {
    printf("fork failed!\n");
    exit(1);
  }
  if (pid == 0) {   // in child
    close(pip_p_to_c[1]);
    close(pip_c_to_p[0]);
    if (read(pip_p_to_c[0], &ch, 1) == 1)
      printf("%d: received ping\n", getpid());
    else
      exit(1);
    write(pip_c_to_p[1], "p", 1);
  } else {  // in parent
    close(pip_p_to_c[0]);
    close(pip_c_to_p[1]);
    write(pip_p_to_c[1], "p", 1);
    if (read(pip_c_to_p[0], &ch, 1) == 1)
      printf("%d: received pong\n", 1);
    else
      exit(1);
    int status;
    wait(&status);
  }
  exit(0);
}