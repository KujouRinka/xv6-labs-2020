#include "user/user.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    write(1, "improper usage of sleep: sleep (time)\n", 38);
    exit(1);
  }
  sleep(atoi(argv[1]));
  exit(0);
}
