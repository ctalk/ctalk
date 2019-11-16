/* $Id: margexprs4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

String class PrefixString;
PrefixString classVariable prefix String NULL;

int main (int argc, char **argv) {

  PrefixString new myPrefixString;
  Integer new i;
  Integer new strLength;

  PrefixString prefix = "string prefix.";

  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut 0, 'S';
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut (0 + 1), 'T';
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut ((0 + 2), 'R');
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut (0 + 3, 'I');
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut 4, (PrefixString prefix at 4) ^ 32;
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut 4, (PrefixString prefix at 4) & ~32;
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut 4, ((PrefixString prefix at 4) ^ 32);
  printf ("%s\n", PrefixString prefix);

  strLength = PrefixString prefix length;
  for (i = 0; i < strLength; i = i + 1) {
    printf ("%c\n", PrefixString prefix at i);
  }

  exit(0);
}
