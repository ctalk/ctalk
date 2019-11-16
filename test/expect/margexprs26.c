
/* Aliasing a char ** to an Array argument of another char ** (which
   is actually passed as a Symbol value) of the parseArgv method. */

Application instanceMethod printArgs (void) {

  self cmdLineArgs map {
    printf ("%s\n", self);
  }
}

Application instanceMethod parseArgv (Integer argc, Array argv) {

  int i;
  char **argv_array;
  
  returnObjectClass Array;

  argv_array = argv value;

  for (i = 0; i < argc; i = i + 1) {

    self cmdLineArgs atPut i, argv_array[i];

  }

}

int main (int argc, char **argv) {

  Application new app;

  app parseArgv argc, argv;

  app printArgs;
}
