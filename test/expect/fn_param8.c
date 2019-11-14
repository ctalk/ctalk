/*
 *  The operand here refers to the operand of __ctalkSystemSignalNumber - 
 *  "SIGINT".
 */
SignalHandler instanceMethod localSetSigInt (void) {
  self addInstanceVariable "sigNo", __ctalkSystemSignalNumber ("SIGINT");
  return self;
}

int main () {
  SignalHandler new s;
  s localSetSigInt;
  printf ("Pass\n");
}
