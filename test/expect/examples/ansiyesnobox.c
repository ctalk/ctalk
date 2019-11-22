int main () {
  ANSIYesNoBoxPane new messageBox;
  String new answer;

/*   messageBox paneStream openOn "/dev/ttya"; */
/*   messageBox paneStream setTty 9600, 1, 'n', 8; */
 
  messageBox withBorder;

  messageBox withText "Are you sure you want to quit?";
  answer = messageBox show 15, 10;
  messageBox cleanup;
  printf ("You answered, \"%s\"\n", answer);
}
