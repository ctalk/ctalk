
String instanceMethod crlf (String textIn) {
  char textOut[1024];
  Integer new i;
  Integer new j;
  Character new c;

  memset (textOut, 0, 1024);
  j = 0;
  for (i = 0; textIn at i; ++i) {
    if (textIn at i == 0) {
      break;
    } else if (textIn at i == '\n') {
      textOut[j] = 13;
      ++j;
      textOut[j]= 10;
      ++j;
    } else {
      /* Check these. */
      textOut[j] = (textIn at i);
      ++j;
    }
  }
  textIn = textOut;
}


int main () {
  String new str;
  Integer new i;
  Integer new length;

  str = "This is the home page.\n\n";

  str += "And this is [AnotherPage].\n\n";

  str += "----\n\n";

  str += "And this is [StillAnotherPage].\n\n";

  str += "This is *bold text.*\n\n";

  str += "This is **emphasized (italic) text.**\n\n";

  str += "This is ***bold-emphasized text.***\n\n";

  str crlf str;

  length = strlen (str);

  for (i = 0; i < length; ++i) {

    switch (str at i)
      {
      case 13:
	printf ("\n");
	break;
      case 10:
	break;
      case '[':
	printf ("[");
	break;
      case ']':
	printf ("]");
	break;
      case '*':
	printf ("*");
	break;
      case '-':
	printf ("-");
	break;
      default:
	printf ("%c", str at i);
	break;
      }

  }

  printf ("%s\n", str);
}
