/* The extra period at the end of the message is intentional. */
#include <string.h>
#include <errno.h>

Integer instanceMethod myMethod (void) {
  Exception new ex;
  ex raiseException INVALID_OPERAND_X,
    "In myMethod test exception: " + strerror (errno) + ".";
}

int main () {
  Integer new myInt;
  Exception new ex;

  myInt myMethod;
  if (ex pending) {
    ex handle;
  }
}
