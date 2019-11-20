#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
int main () {
  struct stat statbuf;
  system ("examples/mkdir1");
  if (!stat ("testDir", &statbuf)) {
    if ((statbuf.st_mode & (S_IRUSR|S_IWUSR|S_IXUSR)) &&
	(statbuf.st_mode & ~(S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH))) {
      printf ("Pass\n\n");
      return 0;
    }
  }
  printf ("Fail\n");
  return 1;
}
