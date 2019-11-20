/* $Id: strlen1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <string.h>

int main () {

  String new myString;
  Integer new strLength;

  myString = "Test string.";

/*   length =  */
/*     strlen (__ctalk_to_c_char_ptr(__ctalk_get_object("myString", "String"))); */
  strLength = strlen (myString);

  if (strLength == 12) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
