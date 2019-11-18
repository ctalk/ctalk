/*
 *  This should generate errors similar to this:
 *  18295: Undefined method, ".".
 *  18296: Undefined method, ".".
 *    In function 'main':
 *   26702: error: request for member '__o_name' in something not a structure or union
 *  26703: error: request for member '__o_classname' in something not a structure or union
 *
 */
int main () {
  Object new o;

  printf ("%s\n", o . __o_name);
  printf ("%s\n", o . __o_classname);
}
