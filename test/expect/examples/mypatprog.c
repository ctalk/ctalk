/* this also has a failed match in it because I want to know if that
   changes when the regex parser gets updated */
int main () {

  String new s;

  s printMatchToks TRUE;

  s = "192.168.0.1";

  if (s =~ /\d+\.(\d+)\.\d+\.\d+/) {
    printf ("match!\n");
  }

  s = "192.a.0.1";

  if (s =~ /\d+\.(\d+)\.\d+\.\d+/) {
    printf ("match!\n");
  } else {
    printf ("non-match!\n");
  }

}
