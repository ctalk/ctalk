/* $Id: lclock.c,v 1.4 2020/10/19 17:42:39 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015 Robert Kiesling, rk3314042@gmail.com.
  Permission is granted to copy this software provided that this copyright
  notice is included in all source code modules.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 51 Franklin St., Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
 *   lclock.c - Another clock program that displays and updates the 
 *   time in large block characters.
 *
 *   Usage: lclock [-s|-t|-h]
 *     -s   Display seconds.
 *     -t   Twelve-hour format.
 *     -h   Print help message and exit.
 */

/*
 *  Terminal sequences for XTERM/ANSI/VT100 terminals.
 *  Change these for other terminal types.
 */
#define VT_REV    "\033[7m"     /* Reverse video.                    */
#define VT_NORM   "\033[0m"     /* Normal video.                     */
#define VT_CPOS   "\033[%d;%df" /* Set cursor position: row, column. */
#define VT_CLS    "\033[2J"     /* Clear the screen.                 */
#define VT_RESET  "\033[!p"     /* Soft reset.                       */

#define DIGIT_WIDTH  6
#define DIGIT_HEIGHT 7

int args (int, char **);   /* Add prototype to avoid a warning. */

static int row = 0,                  /* Row/column display parameters. */
  column = 0, 
  digitN = 0; 

static int displaySeconds = FALSE,   /* Command line options. */
  hours24 = TRUE;

Array class ClockDigit;         /* The clock digit subclass and      */
                                /* clock digits, which are initial-  */
ClockDigit new digit0;          /* ized below.                       */
ClockDigit new digit1;
ClockDigit new digit2;
ClockDigit new digit3;
ClockDigit new digit4;
ClockDigit new digit5;
ClockDigit new digit6;
ClockDigit new digit7;
ClockDigit new digit8;
ClockDigit new digit9;
ClockDigit new colon;

CTime new prevTime;

Application instanceMethod updateTime (void) {
  returnObjectClass Boolean;
  CTime new timeNow;
  timeNow utcTime;

  if (timeNow == prevTime) {
    return False;
  } else {
    prevTime = timeNow;
    return True;
  }
}

Array instanceMethod getTime (void) {

  CTime new timeNow;
  Array new currentLocalTime;

  timeNow utcTime;

  if (!hours24)
    timeNow = timeNow - 43200;

  currentLocalTime = timeNow localTime;

  self atPut 0, (currentLocalTime at 2);
  self atPut 1, (currentLocalTime at 1);
  self atPut 2, (currentLocalTime at 0);

  return NULL;
}

ClockDigit classMethod initDigits (void) {

  digit0 atPut 0, "001110";
  digit0 atPut 1, "010001";
  digit0 atPut 2, "010001";
  digit0 atPut 3, "010001";
  digit0 atPut 4, "010001";
  digit0 atPut 5, "010001";
  digit0 atPut 6, "001110";

  digit1 atPut 0, "000100";
  digit1 atPut 1, "001100";
  digit1 atPut 2, "010100";
  digit1 atPut 3, "000100";
  digit1 atPut 4, "000100";
  digit1 atPut 5, "000100";
  digit1 atPut 6, "011111";

  digit2 atPut 0, "001110";
  digit2 atPut 1, "010001";
  digit2 atPut 2, "000001";
  digit2 atPut 3, "000110";
  digit2 atPut 4, "001000";
  digit2 atPut 5, "010000";
  digit2 atPut 6, "011111";

  digit3 atPut 0, "011111";
  digit3 atPut 1, "000001";
  digit3 atPut 2, "000010";
  digit3 atPut 3, "001111";
  digit3 atPut 4, "000001";
  digit3 atPut 5, "010001";
  digit3 atPut 6, "001110";

  digit4 atPut 0, "000001";
  digit4 atPut 1, "000011";
  digit4 atPut 2, "000101";
  digit4 atPut 3, "001001";
  digit4 atPut 4, "011111";
  digit4 atPut 5, "000001";
  digit4 atPut 6, "000001";

  digit5 atPut 0, "011111";
  digit5 atPut 1, "010000";
  digit5 atPut 2, "010110";
  digit5 atPut 3, "011001";
  digit5 atPut 4, "000001";
  digit5 atPut 5, "010001";
  digit5 atPut 6, "001110";

  digit6 atPut 0, "001110";
  digit6 atPut 1, "010001";
  digit6 atPut 2, "010000";
  digit6 atPut 3, "011110";
  digit6 atPut 4, "010001";
  digit6 atPut 5, "010001";
  digit6 atPut 6, "001110";

  digit7 atPut 0, "011111";
  digit7 atPut 1, "000001";
  digit7 atPut 2, "000010";
  digit7 atPut 3, "000100";
  digit7 atPut 4, "000100";
  digit7 atPut 5, "001000";
  digit7 atPut 6, "001000";

  digit8 atPut 0, "001110";
  digit8 atPut 1, "010001";
  digit8 atPut 2, "010001";
  digit8 atPut 3, "001110";
  digit8 atPut 4, "010001";
  digit8 atPut 5, "010001";
  digit8 atPut 6, "001110";

  digit9 atPut 0, "001110";
  digit9 atPut 1, "010001";
  digit9 atPut 2, "010001";
  digit9 atPut 3, "001111";
  digit9 atPut 4, "000001";
  digit9 atPut 5, "010001";
  digit9 atPut 6, "001110";

  colon atPut 0, "000000";
  colon atPut 1, "001100";
  colon atPut 2, "001100";
  colon atPut 3, "000000";
  colon atPut 4, "001100";
  colon atPut 5, "001100";
  colon atPut 6, "000000";

  return NULL;
}

ClockDigit instanceMethod printDigitLine (void) {

  int i;
  OBJECT *self_var;

  self_var = __ctalk_self_internal ();

  printf (VT_CPOS, row, column);
  for (i = 0; i < DIGIT_WIDTH; i++) {
    if (self_var -> __o_value[i] == '1') {
      printf ("%s %s", VT_REV, VT_NORM);
    } else {
      printf (" ");
    }
  }
  ++row;
  return NULL;
}

ClockDigit instanceMethod printDigit (void) {

  row = 1; 
  column = digitN * (DIGIT_WIDTH + 1);
  self map printDigitLine;

  return NULL;
}

Integer instanceMethod selectDigit (void) {

  returnObjectClass ClockDigit;

  if (self == 0) {return digit0;}
  if (self == 1) {return digit1;}
  if (self == 2) {return digit2;}
  if (self == 3) {return digit3;}
  if (self == 4) {return digit4;}
  if (self == 5) {return digit5;}
  if (self == 6) {return digit6;}
  if (self == 7) {return digit7;}
  if (self == 8) {return digit8;}
  if (self == 9) {return digit9;}

  /* Return something that's not NULL. */
  return digit0;
}

int main (int argc, char **argv) {

  Application new clockApp;
  Array new clockTime;
  Integer new hourDigit1;
  Integer new hourDigit2;
  Integer new minuteDigit1;
  Integer new minuteDigit2;
  Integer new secondDigit1;
  Integer new secondDigit2;
  ClockDigit new digit;


  clockApp enableExceptionTrace;
  clockApp installExitHandlerBasic;
  clockApp installAbortHandlerBasic;

  /*
   *  If there is not a class defined for the 
   *  application, it is often more convenient to 
   *  parse the command line options using C.
   */
  if (argc > 1)          
    args (argc, argv);

  ClockDigit initDigits;


  /* Clear the screen.
   */
  printf (VT_CLS);

  while (1) {

    clockApp uSleep 500000L;

    if (!clockApp updateTime)
       continue;

    clockTime getTime;

    /*
     *  Position the cursor at the upper left-hand 
     *  corner of the screen and display the time.
     */
    printf (VT_CPOS, 1, 1);

    hourDigit1 = (clockTime at 0) / 10;
    hourDigit2 = (clockTime at 0) % 10;
    minuteDigit1 = (clockTime at 1) / 10;
    minuteDigit2 = (clockTime at 1) % 10;
    secondDigit1 = (clockTime at 2) / 10;
    secondDigit2 = (clockTime at 2) % 10;

    if (!hours24 && ((clockTime at 0) < 10)) {
      digitN = 0;
      /* Without parens helps check the compiler. */
      hourDigit2 selectDigit printDigit;
    } else {
      digitN = 0;
      (hourDigit1 selectDigit) printDigit;
      ++digitN;
      (hourDigit2 selectDigit) printDigit;
    }

    ++digitN;
    colon printDigit;

    ++digitN;
    (minuteDigit1 selectDigit) printDigit;
    ++digitN;
    (minuteDigit2 selectDigit) printDigit;

    if (displaySeconds == TRUE) {
      ++digitN;
      colon printDigit;

      ++digitN;
      (secondDigit1 selectDigit) printDigit;
      ++digitN;
      (secondDigit2 selectDigit) printDigit;
    }

    printf ("\n");

  }

  exit (0);
}

void exit_help (void) {
  printf ("Usage: lclock [-s|-t|-h]\n");
  exit (1);
}

int args (int argc, char **argv) {

  int i;

  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1])
	{
	case 's':
	  displaySeconds = TRUE;
	  break;
	case 't':
	  hours24 = FALSE;
	  break;
	case 'h':
	default:
	  exit_help ();
	  break;  /* Not reached. */
	}
    } else {
      exit_help ();
    }
  }
  return 0;
}

