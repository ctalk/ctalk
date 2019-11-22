int main () {
  ANSITerminalStream new term;
  Character new c;

  term rawMode;
  term clear;
  term gotoXY 1, 1;

  while ((c = term getCh) != EOF)
    term printOn "%c", c;

  term restoreTerm;
}
