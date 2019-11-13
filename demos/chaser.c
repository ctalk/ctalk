
String new spaces;
String new msg;
String new fileName;
Boolean new useFile;
Integer new delayUSecs;
Integer new termWidth;

void exit_help (char *);

Application instanceMethod readFile (void) {
  ReadFileStream new readStream;
  String new s;
  returnObjectClass String;

  readStream openOn fileName;

  if (!readStream exists fileName) {
    printf ("chaser: %s: File not found.\n", fileName);
    exit (1);
  }

  s = readStream readAll;

  readStream closeStream;

  s chomp;

  if (s length == 0) {
    s = "chaser: file \"" + fileName + "\" is empty.";
  }

  return s;

}

Application instanceMethod chaserOptions (void) {

  Integer new i;
  Integer new nParams;
  String new param;

  nParams = self cmdLineArgs size;

  for (i = 1; i < nParams; i++) {

    param = self cmdLineArgs at i;

    if (param  == "-f") {
      fileName = self cmdLineArgs at i + 1;
      useFile = true;
      i += 1;
      continue;
    }

    if (param == "-d") {
      delayUSecs = (self cmdLineArgs at i + 1) asInteger;
      i += 1;
      continue;
    }

    if (param == "-w") {
      termWidth = (self cmdLineArgs at i + 1) asInteger;
      i += 1;
      continue;
    }

    if ((param  == "-h") || (param == "--help"))
      exit_help ("chaser");

    msg = self cmdLineArgs at i;

  }
}

int main (int argc, char **argv) {

  Application new chaser;
  ANSITerminalStream new term;
  ANSITerminalPane new pane;
  Integer new len;
  Integer new i;

  useFile = false;
  delayUSecs = 10000;
  termWidth = 0;

  chaser parseArgs  argc, argv;
  chaser chaserOptions;

  /* If the user didn't set the terminal width on the command line,
     then use the terminal's own setting. */
  if (termWidth == 0)
    termWidth = pane terminalWidth;

  if (useFile) {
    msg = chaser readFile;
  }

  for (i = 0; i < termWidth; i++) {
    spaces += " ";
  }

  msg = spaces + msg + " ";

  len = msg length;

  term clear;

  while (TRUE) {
    for (i = 0; i < len; i++) {
      term gotoXY 0, 0;
      printf ("%s", msg subString i, termWidth);
      fflush (stdout);
      chaser uSleep delayUSecs;
    }

    if (useFile) {
      msg = chaser readFile;
      msg = spaces + msg + " ";
      len = msg length;
    }

  }
}

void exit_help (char *cmd) {
  printf ("\nUsage: %s [-h] | [-w <cols> ] [f <file>] | <message>\n", cmd);
  printf ("-d <usecs>   Update delay in microseconds. Default = 10000.\n");
  printf ("-f <file>    Display the text from <file>.\n");
  printf ("-h           Print this message and exit.\n");
  printf ("-w <cols>    Terminal width in character columns.\n");        
  printf ("Please report bugs to: rk3314042@gmail.com.\n");
  exit (0);
}
