
int main () {

  String new testDirName;
  Integer new r;

  testDirName = "testdir";

  if ((r = mkdir (testDirName, 0755)) != 0) 
    printf ("Error.\n");

  if ((r = rmdir (testDirName)) != 0) 
    printf ("Error.\n");

}
