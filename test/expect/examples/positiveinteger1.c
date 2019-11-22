
Integer class PositiveInteger;

PositiveInteger instanceMethod - subtract (int operand) {
  Exception new e;
  returnObjectClass Integer;
  int i, rcvr_value, operand_value;
  rcvr_value = self value;
  operand_value = operand value;
  if ((rcvr_value - operand_value) < 0) {
    e raiseException INVALID_OPERAND_X, "Negative result for PositiveInteger";
    i = 0;
  } else {
    i = rcvr_value - operand_value;
  }
  methodReturnInteger (i);
}

int main () {
  Exception new e;
  PositiveInteger new p;

  p = 1;

  printf ("%d\n", p + 1);

  printf ("%d\n", p - 2);
  if (e pending)
    e handle;
}
