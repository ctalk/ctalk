
#include <stddef.h>

struct _my_struct {
  long int lint;
  int iint;
};

int main (void) {
  struct _my_struct m_s;
  printf ("%d\n", offsetof (struct _my_struct, iint));
}
