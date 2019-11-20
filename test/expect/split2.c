
Object instanceMethod mySplit (String text, Array linesOut) {
  Integer new nItems;
  returnObjectClass Integer;

  nItems = text split (10 asCharacter), linesOut;
}

int main () {

  Integer new nLines;
  Array new lines;
  String new text;
  Object new obj;

  text =  "First line\n";
  text +=  "second line\n";
  text +=  "third line\n";

  nLines = obj mySplit text, lines;

  lines map {
    printf ("%s\n", self);
  }

}
