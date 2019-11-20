/* $Id: super1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/* Test an argblk "super" as a function argument within an argument
   block. 
*/
void exit_loop_func (char *, char *);

Application instanceMethod mapMethod (void) {

  self cmdLineArgs map {
    if (self at 0 == '-') {
      if (self at 1 == 'g') {
	continue;
      }
      if (self at 1 == 'b') {
	continue;
      }
      if (self at 1 == 'f') {
	continue;
      }
      exit_loop_func (super cmdLineArgs at 0, self);
      break;
    }

  }

}

int main (int argc, char **argv) {
  Application new testApp;

  testApp cmdLineArgs atPut 0, "First element.";
  testApp cmdLineArgs atPut 1, "Second element.";
  testApp cmdLineArgs atPut 2, "-Third element.";
  testApp mapMethod;
}

void exit_loop_func (char *super_str, char *self_str) {
  printf ("exit_loop_func: %s --> %s\n", super_str, self_str);
}

