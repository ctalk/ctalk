/* Checks the unknown format conversion warning message. */

Application class MyAppClass;
MyAppClass instanceVariable text String NULL;

#define LF 10

MyAppClass instanceMethod myMethod (void) {
  String new s, s1;

  s1 = self;

  s1 map {
    if (self == LF) {
      self = ' ';
    }
  }

  printf ("New String: %s, and: \n", s, s1);
}

int main () {
  MyAppClass new myApp;

  myApp text = "Hello, world.\nHello, world.";
  myApp myMethod;

}
