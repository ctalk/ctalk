Rectangle instanceMethod printVal (void) {
  printf ("%d %d %d %d\n",
	  (self top start x), (self top start y),
	  (self bottom end x), (self bottom end y));

}

int main () {

  Rectangle new rect;

  rect dimensions 10, 20, 30, 40;
  rect printVal;
}
