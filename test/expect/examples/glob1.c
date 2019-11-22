
List new fileNames;

String instanceMethod globFile (void) {

  DirectoryStream new d;
  List new globFiles;

  if (d hasMeta self) {  /* The argv entry contains a wildcard character. */
    d globPattern self, globFiles;
    globFiles map {
      fileNames push self;
    }
  } else {             /* The argv entry is an unambiguous file name. */
    fileNames push self;
  }

}

int main (int argc, char *argv[]) {
  
  int i;

  for (i = 1; i < argc; ++i) {
    argv[i] globFile;
  }
  
  fileNames map {
    printf ("%s\n", self);
  }
}
