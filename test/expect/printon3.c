/*
 *  Test numeric conversion with __ctalkObjectPrintOn to stdoutStream.
 */

Application new printOnApp;

Application instanceMethod printOnMethod (char *fmt, ...) {
  __ctalkObjectPrintOn (WriteFileStream stdoutStream);
  return NULL;
}

int main () {

  WriteFileStream classInit;

  printOnApp printOnMethod "%c ", 64;
  printOnApp printOnMethod "%c ", 65l;
#ifdef __APPLE__
  printOnApp printOnMethod "%c ", 66;
#else
  printOnApp printOnMethod "%c ", 66ll;
#endif
  printOnApp printOnMethod "%c ", 67L;
  printOnApp printOnMethod "%c ", 'a';
  printOnApp printOnMethod "%c ", "b";
  printOnApp printOnMethod "%c ", 'c';
  printOnApp printOnMethod "\n";

  printOnApp printOnMethod "%i ", 64;
  printOnApp printOnMethod "%i ", 65l;
#ifdef __APPLE__
  printOnApp printOnMethod "%i ", 66;
#else
  printOnApp printOnMethod "%i ", 66ll;
#endif
  printOnApp printOnMethod "%i ", 67L;
  printOnApp printOnMethod "%i ", 'a';
  printOnApp printOnMethod "%i ", "b";
  printOnApp printOnMethod "%i ", 'c';
  printOnApp printOnMethod "\n";

  printOnApp printOnMethod "%o ", 64;
  printOnApp printOnMethod "%o ", 65l;
#ifdef __APPLE__
  printOnApp printOnMethod "%o ", 66;
#else
  printOnApp printOnMethod "%o ", 66ll;
#endif
  printOnApp printOnMethod "%o ", 67L;
  printOnApp printOnMethod "%o ", 'a';
  printOnApp printOnMethod "%o ", "b";
  printOnApp printOnMethod "%o ", 'c';
  printOnApp printOnMethod "\n";

  printOnApp printOnMethod "%#x ", 64;
  printOnApp printOnMethod "%#x ", 65l;
#ifdef __APPLE__
  printOnApp printOnMethod "%#x ", 66;
#else
  printOnApp printOnMethod "%#x ", 66ll;
#endif
  printOnApp printOnMethod "%#x ", 67L;
  printOnApp printOnMethod "%#x ", 'a';
  printOnApp printOnMethod "%#x ", "b";
  printOnApp printOnMethod "%#x ", 'c';
  printOnApp printOnMethod "\n";

  printOnApp printOnMethod "%#X ", 64;
  printOnApp printOnMethod "%#X ", 65l;
#ifdef __APPLE__
  printOnApp printOnMethod "%#X ", 66;
#else
  printOnApp printOnMethod "%#X ", 66ll;
#endif
  printOnApp printOnMethod "%#X ", 67L;
  printOnApp printOnMethod "%#X ", 'a';
  printOnApp printOnMethod "%#X ", "b";
  printOnApp printOnMethod "%#X ", 'c';
  printOnApp printOnMethod "\n";

  printOnApp printOnMethod "%u ", 64;
  printOnApp printOnMethod "%u ", 65l;
#ifdef __APPLE__
  printOnApp printOnMethod "%u ", 66;
#else
  printOnApp printOnMethod "%u ", 66ll;
#endif
  printOnApp printOnMethod "%u ", 67L;
  printOnApp printOnMethod "%u ", 'a';
  printOnApp printOnMethod "%u ", "b";
  printOnApp printOnMethod "%u ", 'c';
  printOnApp printOnMethod "\n";

  printOnApp printOnMethod "%e ", 1.1;
  printOnApp printOnMethod "%E ", 1.1;
  printOnApp printOnMethod "%f ", 1.1;
#ifndef __APPLE__
  printOnApp printOnMethod "%F ", 1.1;
#endif
  printOnApp printOnMethod "\n";

}
