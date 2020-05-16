/* $Id: ctcheckquery.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2008 - 2012 Robert Kiesling, rk3314042@gmail.com.
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
 *   ctcheckquery.c - Print a URL's CGI query parameters.  Written
 *                    for Apache Web servers.
 *
 *   Installation:
 *   1. Build the ctcheckquery program.
 *
 *      ctcc -I . ctcheckquery.c -o ctcheckquery
 *
 *      You must include the current directory in the Ctalk
 *      search path (with -I .), so Ctalk can locate the CGIApp 
 *      class library.
 *
 *   2. Copy the ctcheckquery binary to the server's CGI directory,
 *      and set its permissions. If you don't know where the
 *      CGI directory is, look at the CGI configuration in the 
 *      server's httpd.conf file. For example, for a normal 
 *      Apache 2 installation:
 *
 *      $ su
 *      # cp ctcheckquery /usr/local/apache2/cgi-bin
 *      # chmod 0755 /usr/local/apache2/cgi-bin/ctcheckquery
 *
 *      If necessary, change the owner and group of 
 *      the ctcheckquery binary so the server can execute it.
 *
 *   3. Perform any additional configuration the 
 *      server needs.  In addition to configuring 
 *      the server to run CGI scripts, you might need
 *      to add the ctalk libraries to the server's 
 *      library path, with a line similar to the following,
 *      outside of any directory wrapper.
 *
 *      SetEnv LD_LIBRARY_PATH /usr/local/lib
 * 
 *   4. To run ctcheckquery, browse to a URL like the following.
 *
 *      http://<server-name>/cgi-bin/ctcheckquery?param1=value1&param2=value2
 */

AssociativeArray instanceMethod printQueryParamPair (void) {
  printf ("<tt>%s=", self name);
  printf ("\"%s\"</tt><br>\n", self getValue);
  return NULL;
}

int main (int argc, char **argv, char **envp) {
  CGIApp new queryApp;

  queryApp getServerEnv envp;

  queryApp parseQueryString;

  queryApp httpHeader;
  queryApp htmlPageHeader "HTTP Query";

  queryApp queryValues mapKeys printQueryParamPair;

  return 0;
}

