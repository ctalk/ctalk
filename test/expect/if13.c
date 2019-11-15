

int getmonth (void) {
  return 1;
}

CalendarTime instanceMethod checkMonth (void) {
  if (self month value == getmonth ()) {
    printf ("ok!\n");
  } else {
    printf ("not ok\n");
  }
}

int main () {
  CalendarTime new ct;
  ct month value = 1;
  ct checkMonth;
}
