
/* This checks the combining of expressions that have a variable
   number of paren levels between boolean ops. (The check is in
   parse_ctrlblk_subexprs in ifexpr.c.)
*/

Integer new startx, starty;

int main () {

  Integer new x, y, i, j;

  x = 10;
  startx = 10;
  y = 20;
  starty = 20;
  i = 5;
  j = 3;
  
  if ((x == startx && y == starty) && (i < 10) && (j > 0)) {
    printf ("Pass\n");
  }

  if ((j > 0) && (x == startx && y == starty) && (i < 10)) {
    printf ("Pass\n");
  }
}
