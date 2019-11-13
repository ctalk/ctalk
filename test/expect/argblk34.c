/* Checks that we can process the instance variable "text" as
   the receiver of the argument block in myMethod. In that
   case, we can determine that "self" within the argument
   block is a Character object. */

Application class MyAppClass;
MyAppClass instanceVariable text String NULL;

#define LF 10

MyAppClass instanceMethod myMethod (void) {

  self text map {
    if (self == LF) {
      self = ' ';
    }
  }

  printf ("%s\n", self text);
}

int main () {
  MyAppClass new myApp;

  myApp text = "Hello, world.\nHello, world.";
  myApp myMethod;

}
