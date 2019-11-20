
struct _my_struct {
  int my_int_mbr;
};

int main (void) {
  struct _my_struct m_s;
  List new main_l;
  
  m_s.my_int_mbr = 3;

  main_l = "one", "two", "three", "four";

  main_l map {
    printf ("%d\n", m_s.my_int_mbr);
    ++m_s.my_int_mbr;
  }
}
