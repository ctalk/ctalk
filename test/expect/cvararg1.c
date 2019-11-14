
/* Should complete with no output and no memory leaks. */

Integer class RectInt;
RectInt instanceVariable rect Rectangle NULL;

RectInt instanceMethod shiftRect (Integer y, Integer height) {

  self rect dimensions 2, y, 
    (self rect bottom end x), 
    height;
}

#define HEIGHT 20;

RectInt instanceMethod doShift (void) {

  int i, new_y;

  for (i = 11; i <= 50; i++) {
    new_y = i;
    self shiftRect new_y, HEIGHT;
  }

}

int main () {

  int i, new_x, new_y;

  RectInt new myRect;

  myRect rect dimensions 2, 2, 20, 20;

  myRect doShift;

  printf ("Done.\n");
}
