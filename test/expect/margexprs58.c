/* $Id: margexprs58.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* This is similar to margexprs4.c, but with C variables. */

String class PrefixString;
PrefixString classVariable prefix String NULL;

int main (int argc, char **argv) {

  int i_term, i_term_2, *i_term_p, *i_term_2_p;
  PrefixString new myPrefixString;

  PrefixString prefix = "string prefix.";

  printf ("%s\n", PrefixString prefix);

  i_term = 0;
  i_term_p = &i_term;
  
  PrefixString prefix atPut *i_term_p, 'S';
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut (*i_term_p + 1), 'T';
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut ((*i_term_p + 2), 'R');
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut (*i_term_p + 3, 'I');
  printf ("%s\n", PrefixString prefix);

  i_term = 4;
  PrefixString prefix atPut *i_term_p, (PrefixString prefix at 4) ^ 32;
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut *i_term_p, (PrefixString prefix at 4) & ~32;
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut *i_term_p, ((PrefixString prefix at 4) ^ 32);
  printf ("%s\n", PrefixString prefix);

  i_term_2 = 1;
  i_term_2_p = &i_term_2;
  PrefixString prefix atPut (0 + *i_term_2_p), 'T';
  printf ("%s\n", PrefixString prefix);

  i_term_2 = 2;
  PrefixString prefix atPut ((0 + *i_term_2_p), 'R');
  printf ("%s\n", PrefixString prefix);

  i_term_2 = 3;
  PrefixString prefix atPut (0 + *i_term_2_p, 'I');
  printf ("%s\n", PrefixString prefix);

  i_term = 0;
  i_term_2 = 1;
  PrefixString prefix atPut (*i_term_p + *i_term_2_p), 'T';
  printf ("%s\n", PrefixString prefix);

  i_term_2 = 2;
  PrefixString prefix atPut ((*i_term_p + *i_term_2_p), 'R');
  printf ("%s\n", PrefixString prefix);

  i_term_2 = 3;
  PrefixString prefix atPut (*i_term_p + *i_term_2_p, 'I');
  printf ("%s\n", PrefixString prefix);

  i_term = 4;
  PrefixString prefix atPut 4, (PrefixString prefix at *i_term_p) ^ 32;
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut 4, (PrefixString prefix at *i_term_p) & ~32;
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut 4, ((PrefixString prefix at *i_term_p) ^ 32);
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut *i_term_p, (PrefixString prefix at *i_term_p) ^ 32;
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut *i_term_p, (PrefixString prefix at *i_term_p) & ~32;
  printf ("%s\n", PrefixString prefix);

  PrefixString prefix atPut *i_term_p, ((PrefixString prefix at *i_term_p) ^ 32);
  printf ("%s\n", PrefixString prefix);

  exit(0);
}
