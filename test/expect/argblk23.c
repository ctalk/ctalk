/* $Id: argblk23.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Like argblk21.c, but it checks for argument block translations within
 *  a method, and Object : become stack walkup through multiple argument
 *  blocks.
 */


Integer instanceMethod printAssociativeArray (void) {

  List new l;
  List new l_sub_1;
  List new l_sub_2;
  List new subList;
  AssociativeArray new aArray;
  String new keyStr;
  int i;
  char buf[MAXLABEL];
  char buf2[MAXLABEL];

  l_sub_1 push "l_sub_1-1";
  l_sub_1 push "l_sub_1-2";
  l_sub_2 push "l_sub_2-1";
  l_sub_2 push "l_sub_2-2";

  l push l_sub_1;
  l push l_sub_2;

  i = 0;

  l map { 

    subList become self;

    subList map {

      keyStr become self;

      sprintf (buf, "element %d", i);
      xstrcpy (buf2, buf);

      aArray atPut keyStr, buf2;

      i += 1;
    }

  }

  aArray mapKeys {
    printf ("%s -> %s\n", self name, self getValue value);
  }

  exit(0);
}

int main () {
  Integer new i;

  i printAssociativeArray;
  exit(0);
}
