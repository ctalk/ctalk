/* $Id: floor2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double c_floor;
  Float new floorObj;

  c_floor = floor (3.2);

  if ((floorObj = floor(3.2)) == c_floor) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}