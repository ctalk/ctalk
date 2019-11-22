int main (int argc, char **argv) {

  String new path;
  Array new paths;
  Integer new nItems, i;
  Character new separator;

  if (argc != 2) {
    printf ("Usage: ctpath <path>\n");
    exit (1);
  }

  path = argv[1];

  separator = '/';

  nItems = path split separator, paths;

  for (i = 0; i < nItems; i = i + 1)
    printf ("%s\n", paths at i);

  exit (0);
}

