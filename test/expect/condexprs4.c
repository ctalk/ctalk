
int main () {

  AssociativeArray new a;
  String new s;
  Key new lookahead;

  s = "Hello , world!";

  s tokenize a;

  a mapKeys {

    lookahead = self + 1;

    if (!*lookahead) {
      printf ("(1)%s", *self);
    } else {
      if ((*lookahead) == ",") {
	printf ("(2)%s", *self);
      } else {
        printf ("(3)%s ", *self);
      }
    }
  }
  printf ("\n");

  a mapKeys {

    lookahead = self + 1;

    if (!*lookahead) {
      printf ("(1)%s", *self);
    } else {
      if (*lookahead == ",") {
	printf ("(2)%s", *self);
      } else {
        printf ("(3)%s ", *self);
      }
    }
  }
  printf ("\n");

  a mapKeys {

    lookahead = self + 1;

    if (!*lookahead) {
      printf ("(1)%s", *self);
    } else {
      if (**lookahead == ',') {
	printf ("(2)%s", *self);
      } else {
        printf ("(3)%s ", *self);
      }
    }

  }
  printf ("\n");

  a mapKeys {

    lookahead = self + 1;

    if (!*lookahead) {
      printf ("(1)%s", *self);
    } else {
      if ((*lookahead == ",") || (*lookahead == "!")) {
	printf ("(2)%s", *self);
      } else {
        printf ("(3)%s ", *self);
      }
    }
  }
  printf ("\n");

  a mapKeys {

    lookahead = self + 1;

    if (!*lookahead) {
      printf ("(1)%s", *self);
    } else {
      if (*lookahead == "," || *lookahead == "!") {
	printf ("(2)%s", *self);
      } else {
        printf ("(3)%s ", *self);
      }
    }
  }
  printf ("\n");
}
