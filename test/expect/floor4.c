/* $Id: floor4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double c_floor;
  Float new floorObj;
  Float new argObj;

  argObj = 3.2;
  c_floor = floor (argObj);

  if ((floorObj = floor(argObj)) == c_floor) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
