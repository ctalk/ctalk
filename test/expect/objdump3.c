#include <stdlib.h>

int main () {
  WriteFileStream new w;
  w openOn "WriteFileStream.dump";
  w dump;
  system ("/bin/cat WriteFileStream.dump");
}
