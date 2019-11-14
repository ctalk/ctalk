/*
 *  The operand here refers to the operand of __ctalkSystemSignalNumber - 
 *  "SIGINT".
 */
SignalHandler instanceMethod localSetSigInt (void) {
  Integer new sigNoVar;
  sigNoVar = __ctalkSystemSignalNumber ("SIGINT");
  self addInstanceVariable "sigNo", sigNoVar;
  return self;
}

int main () {
  SignalHandler new s;
  s localSetSigInt;
  printf ("Pass\n");
}
