/* $Id: regex.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of ctalk.
  Copyright © 2018 Robert Kiesling, rkiesling@users.sourceforge.net
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

#ifndef _REGEX_H
#define _REGEX_H

#define METACHARS "*$^.+|?"

#define IS_CHAR_CLASS_ABBREV(c) (c == 'W' ||  c == 'd' || c == 'p' ||	\
				 c == 'w' || c == 'l' || c == 'x')

#define RIGHT_ASSOC_META(c) (c == '*' || c == '+' || c == '|' || c == '?')

#define METACHARACTER_LESS_DOT(c) (c == '^' || c == '$' || c == '*' || \
				   c == '\\' || c == '+' || c == '|' || \
				   c == '?')

/* Attributes used by re_lexical, and  __ctalkMatchText. */
/* Also defined in message.h */
#ifndef META_BROPEN
#define META_BROPEN             (1 << 0)
#endif
#ifndef META_BRCLOSE
#define META_BRCLOSE            (1 << 1)
#endif
#ifndef META_CHAR_CLASS
#define META_CHAR_CLASS         (1 << 2)
#endif
#ifndef META_CHAR_LITERAL_ESC
#define META_CHAR_LITERAL_ESC   (1 << 3)
#endif
#endif /* _REGEX_H */

