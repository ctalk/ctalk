#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
int main () {
  struct stat statbuf;
  int errno_num;
  system ("examples/rmdir1");
  if (stat ("testDir", &statbuf)) {
    errno_num = errno;
    if (errno_num == ENOENT) {
      printf ("rmdir1: %s\n", strerror (errno));
      printf ("Pass\n\n");
      return 0;
    }
  }
  printf ("rmdir1: %d %s\n", errno_num, strerror (errno));
  printf ("Fail\n");
  return 1;
}
