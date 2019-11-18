/*
 *  Test numeric conversion with generic printOn to WriteFileStream
 */

int main () {

  WriteFileStream new s;

  s openOn "printon2.out";

  s printOn "%c\n", 64;
  s printOn "\t";
  s printOn "%c\n", 65l;
  s printOn "\t";
#ifdef __APPLE__
  s printOn "%c\n", 66;
#else
  s printOn "%c\n", 66ll;
#endif
  s printOn "\t";
  s printOn "%c\n", 67L;
  s printOn "\t";
  s printOn "%c\n", 'a';
  s printOn "\t";
  s printOn "%c\n", "b";
  s printOn "\t";
  s printOn "%c\n", 'c';
  s printOn "\n";

  s printOn "%i\n", 64;
  s printOn "\t";
  s printOn "%i\n", 65l;
  s printOn "\t";
#ifdef __APPLE__
  s printOn "%i\n", 66;
#else
  s printOn "%i\n", 66ll;
#endif
  s printOn "\t";
  s printOn "%i\n", 67L;
  s printOn "\t";
  s printOn "%i\n", 'a';
  s printOn "\t";
  s printOn "%i\n", "b";
  s printOn "\t";
  s printOn "%i\n", 'c';
  s printOn "\n";

  s printOn "%o\n", 64;
  s printOn "\t";
  s printOn "%o\n", 65l;
  s printOn "\t";
#ifdef __APPLE__
  s printOn "%o\n", 66;
#else
  s printOn "%o\n", 66ll;
#endif
  s printOn "\t";
  s printOn "%o\n", 67L;
  s printOn "\t";
  s printOn "%o\n", 'a';
  s printOn "\t";
  s printOn "%o\n", "b";
  s printOn "\t";
  s printOn "%o\n", 'c';
  s printOn "\n";

  s printOn "0x%x\n", 64;
  s printOn "\t";
  s printOn "0x%x\n", 65l;
  s printOn "\t";
#ifdef __APPLE__
  s printOn "0x%x\n", 66;
#else
  s printOn "0x%x\n", 66ll;
#endif
  s printOn "\t";
  s printOn "0x%x\n", 67L;
  s printOn "\t";
  s printOn "0x%x\n", 'a';
  s printOn "\t";
  s printOn "0x%x\n", "b";
  s printOn "\t";
  s printOn "0x%x\n", 'c';
  s printOn "\n";

  s printOn "0X%X\n", 64;
  s printOn "\t";
  s printOn "0X%X\n", 65l;
  s printOn "\t";
#ifdef __APPLE__
  s printOn "0X%X\n", 66;
#else
  s printOn "0X%X\n", 66ll;
#endif
  s printOn "\t";
  s printOn "0X%X\n", 67L;
  s printOn "\t";
  s printOn "0X%X\n", 'a';
  s printOn "\t";
  s printOn "0X%X\n", "b";
  s printOn "\t";
  s printOn "0X%X\n", 'c';
  s printOn "\n";

  s printOn "%u\n", 64;
  s printOn "\t";
  s printOn "%u\n", 65l;
  s printOn "\t";
#ifdef __APPLE__
  s printOn "%u\n", 66;
#else
  s printOn "%u\n", 66ll;
#endif
  s printOn "\t";
  s printOn "%u\n", 67L;
  s printOn "\t";
  s printOn "%u\n", 'a';
  s printOn "\t";
  s printOn "%u\n", "b";
  s printOn "\t";
  s printOn "%u\n", 'c';
  s printOn "\n";

  s printOn "%e\n", 1.1;
  s printOn "\t";
  s printOn "%E\n", 1.1;
  s printOn "\t";
  s printOn "%f\n", 1.1;
  s printOn "\t";
#ifndef __APPLE__
  s printOn "%F\n", 1.1;
  s printOn "\n";
#endif

  s closeStream;
}
