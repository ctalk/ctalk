
int main () {
  Application new myApp;
  String new str, output;
  
  str = "/bin/ls examples/ansiterminalstream*.c examples/tree*";
  myApp execC str, output;
  printf ("%s\n", output);

  str = "/bin/ls -l \"examples/search*\"";
  myApp execC str, output;
  printf ("%s\n", output);

  str = "/bin/ls 'examples/tree*'";
  myApp execC str, output;
  printf ("%s\n", output);
}
