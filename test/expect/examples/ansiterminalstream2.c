int main () {
  ANSITerminalStream new term;
  Character new c;
  InputEvent new iEvent;

  term rawMode;
  term openInputQueue;

  while ((c = term getCh) != EOF) {
    iEvent become (term nextInputEvent);
    if (iEvent eventClass == KBDCHAR) {
      term printOn "<KEY>%c", iEvent eventData;
    }
    if (iEvent eventClass == KBDCUR) {
      term printOn "<CURSOR>%c", iEvent eventData;
    }
  }
}
