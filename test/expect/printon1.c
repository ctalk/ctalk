/*
 *  Test numeric conversion with printOn to String.
 */

int main () {

  String new s;

  s printOn "%c", 64;
  printf ("%s\t", s);
  s printOn "%c", 65l;
  printf ("%s\t", s);
#ifdef __APPLE__
  s printOn "%c", 66;
#else
  s printOn "%c", 66ll;
#endif
  printf ("%s\t", s);
  s printOn "%c", 67L;
  printf ("%s\t", s);
  s printOn "%c", 'a';
  printf ("%s\t", s);
  s printOn "%c", "b";
  printf ("%s\t", s);
  s printOn "%c", 'c';
  printf ("%s\t", s);
  printf ("\n");

  s printOn "%i", 64;
  printf ("%s\t", s);
  s printOn "%i", 65l;
  printf ("%s\t", s);
#ifdef __APPLE__
  s printOn "%i", 66;
#else
  s printOn "%i", 66ll;
#endif
  printf ("%s\t", s);
  s printOn "%i", 67L;
  printf ("%s\t", s);
  s printOn "%i", 'a';
  printf ("%s\t", s);
  s printOn "%i", "b";
  printf ("%s\t", s);
  s printOn "%i", 'c';
  printf ("%s\t", s);
  printf ("\n");

  s printOn "%o", 64;
  printf ("%s\t", s);
  s printOn "%o", 65l;
  printf ("%s\t", s);
#ifdef __APPLE__
  s printOn "%o", 66;
#else
  s printOn "%o", 66ll;
#endif
  printf ("%s\t", s);
  s printOn "%o", 67L;
  printf ("%s\t", s);
  s printOn "%o", 'a';
  printf ("%s\t", s);
  s printOn "%o", "b";
  printf ("%s\t", s);
  s printOn "%o", 'c';
  printf ("%s\t", s);
  printf ("\n");

  s printOn "%#x", 64;
  printf ("%s\t", s);
  s printOn "%#x", 65l;
  printf ("%s\t", s);
#ifdef __APPLE__
  s printOn "%#x", 66;
#else
  s printOn "%#x", 66ll;
#endif
  printf ("%s\t", s);
  s printOn "%#x", 67L;
  printf ("%s\t", s);
  s printOn "%#x", 'a';
  printf ("%s\t", s);
  s printOn "%#x", "b";
  printf ("%s\t", s);
  s printOn "%#x", 'c';
  printf ("%s\t", s);
  printf ("\n");

  s printOn "%#X", 64;
  printf ("%s\t", s);
  s printOn "%#X", 65l;
  printf ("%s\t", s);
#ifdef __APPLE__
  s printOn "%#X", 66;
#else
  s printOn "%#X", 66ll;
#endif
  printf ("%s\t", s);
  s printOn "%#X", 67L;
  printf ("%s\t", s);
  s printOn "%#X", 'a';
  printf ("%s\t", s);
  s printOn "%#X", "b";
  printf ("%s\t", s);
  s printOn "%#X", 'c';
  printf ("%s\t", s);
  printf ("\n");

  s printOn "%u", 64;
  printf ("%s\t", s);
  s printOn "%u", 65l;
  printf ("%s\t", s);
#ifdef __APPLE__
  s printOn "%u", 66;
#else
  s printOn "%u", 66ll;
#endif
  printf ("%s\t", s);
  s printOn "%u", 67L;
  printf ("%s\t", s);
  s printOn "%u", 'a';
  printf ("%s\t", s);
  s printOn "%u", "b";
  printf ("%s\t", s);
  s printOn "%u", 'c';
  printf ("%s\t", s);
  printf ("\n");

  s printOn "%e", 1.1;
  printf ("%s\t", s);
  s printOn "%E", 1.1;
  printf ("%s\t", s);
  s printOn "%f", 1.1;
  printf ("%s\t", s);
#ifndef __APPLE__
  s printOn "%F", 1.1;
  printf ("%s\t", s);
#endif
  printf ("\n");

}
