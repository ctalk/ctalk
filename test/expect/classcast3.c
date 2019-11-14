
int main () {

  TreeNode new head;
  TreeNode new sibling;
  String new content;
  TreeNode new tSib;
  TreeNode new tChild;
  Integer new i;

  content = "Head Node";

  head setContent content;

  printf ("%s\n", head content);

  for (i = 1; i <= 10; i++) {
    content = "1. Sibling Node " + (i asString);

    tSib = tSib basicNew content, "TreeNode", "Symbol", content;
    tSib setContent content;
    head makeSibling tSib;

    content = "Child Node";
    tChild = TreeNode basicNew content;
    tChild setContent content;
    tSib addChild tChild;

  }

  head siblings map {
    content = "Child Node 2";
    tChild = TreeNode basicNew content, content;
    tChild setContent content;
    (TreeNode *)self addChild tChild;
  }

  printf ("--------------------\n");

  head map {
    printf ("%s\n", (TreeNode *)self content);
  }
  
}
