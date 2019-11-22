Integer class InitializedInteger;

InitializedInteger instanceMethod new (String newObjectName) {
  InitializedInteger super new newObjectName;
  newObjectName = 0;
  return newObjectName;
}

int main () {
  InitializedInteger new i;
  printf ("%d\n", i);
}
