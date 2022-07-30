#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(2, "xargs: insufficient params\n");
    exit(1);
  }
  char last_args[1024] = {0};
  int sz = 0;
  while (1) {
    if (1024 - sz < 0) {
      fprintf(2, "xargs: input too long\n");
      exit(1);
    }
    int n = read(0, last_args + sz, 1024 - sz);
    if (n == 0)
      break;
    sz += n;
  }
  if (sz > 0 && last_args[sz - 1] == '\n')
    last_args[sz - 1] = '\0';
  char *pf = last_args;
  char *pb = last_args;
  int once = 0;
  while (!once || *pf != '\0') {
    once = 1;
    while (*pf == '\n')
      ++pf;
    pb = pf;
    while (*pb != '\0' && *pb != '\n')
      ++pb;
    int pid = fork();
    if (pid == -1) {
      fprintf(2, "xargs: fork failed!\n");
      exit(1);
    }
    if (pid == 0) {   // in child
      char buf[512] = {0};
      char *arg_buf[MAXARG] = {0};
      strncpy(buf, pf, pb - pf);
      int i = 0;
      for (; i < argc - 1; ++i)
        arg_buf[i] = argv[i + 1];
      arg_buf[i] = buf;
      exec(argv[1], arg_buf);
      fprintf(2, "xargs: exec failed!\n");
      exit(1);
    } else {  // in parent
      int status;
      wait(&status);
    }
    pf = pb;
  }
  exit(0);
}
