#include "kernel/param.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

char path_name[MAXPATH];
const char *what;

static void path_join(char *path_name, const char *join) {
  uint path_len = strlen(path_name);
  if (path_len + strlen(join) > MAXPATH) {
    fprintf(2, "path name too long\n");
    exit(1);
  }
  if (path_name[0] != '\0')
    strcat(path_name, "/");
  strcat(path_name, join);
}

static void find(const char *cur_path) {
  path_join(path_name, cur_path);
  if (strcmp(cur_path, what) == 0)
    printf("%s\n", path_name);
  int fd = open(path_name, 0);
  if (fd == -1) {
    fprintf(2, "find: cannot open: %s\n", path_name);
    return;
  }
  struct stat st;
  if (fstat(fd, &st) == -1) {
    fprintf(2, "find: cannot fstat: %s\n", path_name);
    return;
  }
  if (st.type == T_DIR) {
    struct dirent dir;
    while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
      if (dir.inum == 0)
        continue;
      char buf[DIRSIZ + 1];
      memmove(buf, dir.name, DIRSIZ);
      buf[DIRSIZ] = '\0';
      if (strcmp(buf, ".") == 0 || strcmp(buf, "..") == 0)
        continue;
      find(buf);
    }
  }
  // pop file_name
  char *p = path_name + strlen(path_name);
  while (*--p != '/') {}
  *p = '\0';
  close(fd);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(2, "improper usage of find: find (path) (what)\n");
    exit(1);
  }
  const char *path = argv[1];
  what = argv[2];
  find(path);
  exit(0);
}
