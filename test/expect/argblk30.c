/* $Id: argblk30.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Check the basic scalar types and char *'s in arg blocks.
 *  
 */


Integer instanceMethod printAssociativeArray (void) {

  AssociativeArray new aArray;
  List new l;
  List new l_sub_1;
  List new l_sub_2;
  List new subList;
  String new keyStr;
  int i;
  double d;
  char c;
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

      buf = "element " + i asString;

      aArray atPut keyStr, buf;

      i += 1;
    }

  }

  c = i + 1;

  l map {

    subList become self;

    subList map {

      keyStr become self;

      buf = "element " + c asString;

      aArray atPut keyStr, buf;

      c += 1;
    }

  }

  d = c + 1;

  l map {

    subList become self;

    subList map {

      keyStr become self;

      buf = "element " + d asString;

      aArray atPut keyStr, buf;

      d += 1;
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
