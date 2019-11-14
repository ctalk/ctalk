
/* Another check for the class cast at the bottom.  The rest is
   all extra checking. */

int main () {

  TreeNode new head;
  TreeNode new sibling;
  String new content;
  Symbol new sib;
  Symbol new child;
  TreeNode new tSib;
  TreeNode new tChild;
  Integer new i;

  content = "Head Node";

  head setContent content;

  printf ("%s\n", head content);

  for (i = 1; i <= 10; i++) {
    content = "2. Sibling Node " + (i asString);

    tSib = tSib basicNew content, "TreeNode", "Symbol", content;
    tSib setContent content;
    head makeSibling tSib;

    content = "Child Node";
    tChild = TreeNode basicNew content;
    tChild setContent content;
    tSib addChild tChild;
  }

  head siblings map {
    *sib = self;
    content = "Child Node 2";
    tChild = TreeNode basicNew content, content;
    tChild setContent content;
    (*sib) addChild tChild;
  }

  printf ("--------------------\n");

  head map {
    printf ("%s\n", (TreeNode *)self content);
  }
  

}
