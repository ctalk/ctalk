

/* Not really an example program, but it's here anyway. */

int main () {

  TreeNode new head;
  TreeNode new head2;
  TreeNode new sibling;
  String new content;
  Symbol new sib;
  Symbol new child;
  TreeNode new tSib;
  TreeNode new tChild;
  TreeNode new tChildChild;
  TreeNode new tChildChildChild;
  Integer new i;
  String new formattedTree;

  content = "Head Node";

  head setContent content;

  head2 setContent content;

  printf ("%s\n", head content);

  for (i = 1; i <= 10; i++) {
    content = "1. Sibling Node " + (i asString);

    *sib = TreeNode basicNew content, content;
    *sib setContent content;
    head makeSibling *sib;

    content = "Child Node";
    tChild = TreeNode basicNew content, content;
    tChild setContent content;
    (*sib) addChild tChild;

    content = "2. Sibling Node " + (i asString);

    tSib = tSib basicNew content, "TreeNode", "Symbol", content;
    tSib setContent content;
    head2 makeSibling tSib;

    content = "Child Node";
    tChild = TreeNode basicNew content;
    tChild setContent content;
    tSib addChild tChild;

  }

  head2 siblings map {
    content = "Child Node 2";
    tChild = TreeNode basicNew content, content;
    tChild setContent content;
    tSib = TreeNode basicNew "Sibling of Child";
    tSib setContent "Sibling of Child";
    tChild makeSibling tSib;
    (TreeNode *)self addChild tChild;

    content = "Child of Child";
    tChildChild = TreeNode basicNew content, content;
    tChildChild setContent content;
    tChild addChild tChildChild;

  }

  printf ("--------------------\n");

  tChildChild = head2 search "Child of Child";

  content = "Child of Child of Child";
  tChildChildChild = TreeNode basicNew content, content;
  tChildChildChild setContent content;
  tChildChild addChild tChildChildChild;
  
  formattedTree = head2 format;
  printf ("%s\n", formattedTree);

}
