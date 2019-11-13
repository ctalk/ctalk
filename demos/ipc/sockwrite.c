/*
  Writes data to the socket that the sockread.c program in this
  subdirectory listens on.

  Build with the command:
   
     ctcc sockwrite.c -o sockwrite

  For more information, refer to the UNIXNetworkStream* classes
  in the Ctalk Language Reference.

 */


int main (int argc, char **argv) {
  UNIXNetworkStreamWriter new writer;
  SystemErrnoException new ex;
  String new sockName;
  String new data;
  int i;

  if (argc < 2) {
    printf ("usage: sockwrite <text> - Writes <text> to sockread's "
	    "socket connection.\n\n"
	    "If you want to include spaces in the output, enclose the "
	    "entire text in quotes: i.e., \"First connect.\"\n\n"
	    "To enter a literal newline, type a backslash ('\\'), and then "
	    "press the Enter key.\n\n"
	    "Typing, \"sockwrite quit,\" exits the sockread program.\n");
    exit (1);
  }

  sockName = "testsocket";

  writer makeSocketPath sockName;
  printf ("writer socket:  %s\n", writer socketPath);

  writer open;
  if (ex pending) {
    ex handle;
    exit (1);
  }

  for (i = 1; i < argc; ++i)
    writer sockWrite argv[i];

  writer closeSocket;

  if (ex pending) {
    ex handle;
    exit (1);
  } else {
    exit (0);
  }
}
