
/* Mainly we want to test the cast in the final map body at the bottom. 
   The rest is mostly extra testing - the receiver casts come in other
   test programs. */

int main () {

  TreeNode new head;
  TreeNode new head2;
  TreeNode new sibling;
  String new content;
  Symbol new sib;
  Symbol new child;
  TreeNode new tSib;
  TreeNode new tChild;
  Integer new i;

  content = "Head Node";

  head setContent content;

  head2 setContent content;

  printf ("%s\n", head content);

  for (i = 1; i <= 10; i++) {
    content = "1. Sibling Node " + (i asString);

    /* *sib = TreeNode basicNew content, content; */
    /* *sib setContent content; */
    /* head makeSibling *sib; */

    /* content = "Child Node"; */
    /* tChild = TreeNode basicNew content, content; */
    /* tChild setContent content; */
    /* *sib addChild tChild; */

    content = "2. Sibling Node " + (i asString);

    tSib = tSib basicNew content, "TreeNode", "Symbol", content;
    tSib setContent content;
    head2 makeSibling tSib;

    content = "Child Node";
    tChild = TreeNode basicNew content;
    tChild setContent content;
    tSib addChild tChild;

  }

  /***/
  /* head siblings map { */
  /*   *tSib = self; */
  /*   content = "Child Node 2"; */
  /*   tChild = TreeNode basicNew content, content; */
  /*   tChild setContent content; */
  /*   tSib addChild tChild; */
  /* } */

  head2 siblings map {
    tSib = self;
    content = "Child Node 2";
    tChild = TreeNode basicNew content, content;
    tChild setContent content;
    /***/
    /* (*tSib) addChild tChild; */
    tSib addChild tChild;
  }

  printf ("--------------------\n");

  head2 siblings map {

    printf ("%s\n", self content);

    (TreeNode *)self children map {
      printf ("  %s\n", self content);
    }

  }

}
