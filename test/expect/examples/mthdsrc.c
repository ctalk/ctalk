int main () {

  Application new app;
  String new methodStr;
  List new tokens;
  
  /* The first argument to methodSource is the class name, and
     the second argument is the method name. */
  methodStr = app methodSource "Object", "basicNew";

  methodStr tokenize tokens;

  tokens map {
    printf ("%s ", self);  /* This example simply prints the method's
                              tokens, but you can perform any processing
                              that you want here. */
  }
  printf ("\n");
}

