int main () {

  ReadFileStream new infileStream;
  Array new pathDirs;
  Integer new nDirs;
  SystemErrnoException new e;

  /* Substitute the path of the file here. */
  infileStream openOn "/your/path/here";
  if (e pending)
    e handle;

  nDirs = infileStream streamPath split '/', pathDirs;

  printf ("%i\n", nDirs);
  printf ("%s\n", pathDirs at 0);
  printf ("%s\n", pathDirs at 1);
  printf ("%s\n", pathDirs at 2);
}
