/* $Id: ctenv.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2008 - 2012, 2015 Robert Kiesling, rk3314042@gmail.com.
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
 *   ctenv.c - Print the environment variables of an Apache
 *             Web server.
 *
 *   Installation:
 *   1. Build the ctenv program.
 *
 *      ctalk -I . ctenv.c -o ctenv.i && gcc ctenv.i -o ctenv -lctalk
 *
 *      or simply,
 *
 *      ctcc -I . ctenv.c -o ctenv
 *
 *      You must include the current directory in the Ctalk
 *      search path (with -I .), so Ctalk can locate the CGIApp 
 *      class library.
 *
 *   2. Copy the ctenv binary to the server's CGI directory,
 *      and set its permissions. If you don't know where the
 *      CGI directory is, look at the CGI configuration in the 
 *       server's httpd.conf file. For example, for a normal 
 *       Apache 2 installation:
 *
 *      $ su
 *      # cp ctenv /usr/local/apache2/cgi-bin
 *      # chmod 0755 /usr/local/apache2/cgi-bin/ctenv
 *
 *      If necessary, change the owner and group of 
 *      the ctenv binary so the server can execute it.
 *
 *   3. Perform any additional configuration the 
 *      server needs.  In addition to configuring 
 *      the server to run CGI scripts, you might need
 *      to add the ctalk libraries to the server's 
 *      library path, with a line similar to the following,
 *       outside of any directory wrapper.
 *
 *      SetEnv LD_LIBRARY_PATH /usr/local/lib
 * 
 *   4. To run ctenv, browse to a URL like the following.
 *
 *      http://<your-web-server-name>/cgi-bin/ctenv
 */

CGIApp new envApp;

AssociativeArray instanceMethod printEnvVar (void) {
  String new keyName;
  String new keyValue;
  /*
   *  Each member of an AssociativeArray is a Key object,
   *  so the actual receiver of a method called inline
   *  by "map" (class AssociativeArray) is a Key.
   *
   *  The method, "name" (class Object) creates an object
   *  as a return value. The method needs to store it
   *  in keyName so the program can delete the object when
   *  it is no longer needed.  Ctalk still does not, at this
   *  time, use "self" in some C expressions, so the method stores 
   *  the result of the "getValue" method (class Key) in keyValue.
   */
  keyName = self name;
  printf ("<tt>%s=", keyName);
  keyValue = self getValue;
  printf ("\"%s\"</tt><br>\n", keyValue);
  return NULL;
}

int main (int argc, char **argv, char **envp) {

  envApp getServerEnv envp;

  envApp httpHeader;
  envApp htmlPageHeader "Server Environment";

  envApp serverEnvironment mapKeys printEnvVar;

  return 0;
}

