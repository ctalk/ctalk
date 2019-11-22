
#include <stdio.h>

int main () {

  ANSITextBoxPane new textPane;

  textPane appendLine "Text pane text.";
  textPane appendLine "Text pane text line 2.";
  textPane appendLine "Text pane text line 3.";

  textPane show 5, 3;
}
