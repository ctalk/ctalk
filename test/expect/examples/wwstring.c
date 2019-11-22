/*  this is the example from the ctalkclasses (1) man page. */

String class WordWrappedString;

WordWrappedString instanceVariable remaining Integer 0;
WordWrappedString classVariable leftMargin Integer 0;
WordWrappedString classVariable rightMargin Integer 0;
WordWrappedString classVariable leftMarginText String "";

WordWrappedString classMethod classInit (void) {
  "Class initialization only needs to initialize the
   stdoutStream file stream so we can print to the terminal."
  WriteFileStream classInit;
}

WordWrappedString classMethod setMargins (Integer leftMarginArg,
					  Integer rightMarginArg) {
  Integer new i;
  self leftMargin = leftMarginArg;
  self rightMargin = rightMarginArg;

  self leftMarginText = "";
  for (i = 0; i < self leftMargin; ++i) {
    self leftMarginText += " ";
  }

}

WordWrappedString instanceMethod output (void) {

  stdoutStream printOn "%s%s\n", self leftMarginText, self;
}

WordWrappedString instanceMethod outputWithMargins (void) {
  Integer new lineLength;
  Integer new lineStart;
  WordWrappedString new lineText;

  lineLength = self rightMargin - self leftMargin;
  self remaining = self length;
  lineStart = 0;
  while (self remaining > lineLength) {
    lineText = self subString lineStart, lineLength;
    lineText output;
    lineStart += lineLength;
    self remaining -= lineLength;
  }
  lineText = self subString ((self length) - (self remaining)), lineLength;
  lineText output;
}


int main () {
  WordWrappedString new str;

  WordWrappedString classInit;
  WordWrappedString setMargins 10, 45;

  str = "We do not claim that the portrait herewith presented ";
  str += "is probable; we confine ourselves to stating that ";
  str += "it resembles the original.  -- Les Miserables.";

  str outputWithMargins;
}
