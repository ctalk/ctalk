/* $Id: argblk26.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Like argblk21.c, but it checks for lval assignments translations within
 *  a method argblk, and Object : become stack walkup through multiple argument
 *  blocks.
 */


Integer instanceMethod printAssociativeArray (void) {

  AssociativeArray new aArray;
  List new l;
  List new l_sub_1;
  List new l_sub_2;
  List new subList;
  String new keyStr;
  String new valStr;
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

      valStr = "element ";
      buf = valStr + i asString;

      aArray atPut keyStr, buf;

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
