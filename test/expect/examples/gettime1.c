Array instanceMethod getTime (void) {

  CTime new timeNow;
  Array new currentLocalTime;

  timeNow utcTime;

  currentLocalTime = timeNow localTime;

  self atPut 0, (currentLocalTime at 2);
  self atPut 1, (currentLocalTime at 1);
  self atPut 2, (currentLocalTime at 0);

  return NULL;
}

int main () {

  Array new clockTime;

  clockTime getTime;

  printf ("%02d:%02d:%02d\n", (clockTime at 0), (clockTime at 1), (clockTime at 2));
}
