/* $Id: floorl4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double c_floor;
  Float new floorObj;
  Float new argObj;

  argObj = 3.2;
  c_floor = floorl (argObj);

  if ((floorObj = floorl(argObj)) == c_floor) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
