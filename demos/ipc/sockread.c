/*
  Reads data from the socket that the sockwrite.c program in this
  subdirectory has written.

  Build with the command:
   
     ctcc sockread.c -o sockread

  For more information, refer to the UNIXNetworkStream* classes
  in the Ctalk Language Reference.

 */

int main () {
  UNIXNetworkStreamReader new reader;
  SystemErrnoException new ex;
  FileStream new f;
  String new sockName;
  String new data;

  sockName = "testsocket";

  reader makeSocketPath sockName;
  printf ("reader socket:  %s\n", reader socketPath);

  /* Delete a socket from a previous connection if necessary. */
  if (f exists reader socketPath) {
    f deleteFile reader socketPath;
  }

  reader open;
  if (ex pending) {
    ex handle;
    unlink (reader socketPath);
    return -1;
  }

  while (1) {
    data = reader sockRead;

    if (data length > 0) {
      printf ("%s", data);
      fflush (stdout);
    }
    if (data == "quit") {
      printf ("\n");
      break;
    }
    if (ex pending) {
      ex handle;
      break;
    }
    usleep (1000);
  }


  reader closeSocket;
  reader removeSocket;
  return 0;
}
