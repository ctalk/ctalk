
int main () {
  ReadFileStream new readF;
  WriteFileStream new writeF;
  FileStream new f;
  Vector new vec;
  LongInteger new size1;
  LongInteger new size2;

  readF openOn "construction.jpeg";
  readF statStream;
  size1 = readF streamSize;
  
  vec = readF readVec size1;
  writeF openOn "construction2.jpeg";
  writeF writeVec vec;

  writeF closeStream ;
  readF closeStream;

  readF statFile "construction2.jpeg";
  size2 = readF size;

  if (size1 == size2) {
    printf ("Pass: ");
  } else {
    printf ("Fail: ");
  }
  printf ("file1 size: %d; file2 size: %d\n", size1, size2);

}
