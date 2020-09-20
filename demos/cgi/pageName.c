/* $Id: pageName.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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
 *   pageName.c - Retrieve a Web page using a symbolic name.
 *
 *   pageName reads the value of its query from a file of 
 *   <page name>|<url> records, and returns the page that 
 *   matches <page name>.  
 *
 *   This allows you to refer to pages using a pageName label, 
 *   and if you need to change a page's URL, only the pageName
 *   entry needs to be edited.
 *
 *   The pageName name/url records are stored in a text file in
 *   the Web server's document directory.  The file is named, 
 *   http://<web-server>/__page_names.  You can change the name 
 *   by editing the PAGEFILENAME definition below.
 *
 *   Each line of the __page_names file has the format:
 *
 *     <page_name>|<url>|<content-type>
 *
 *   An example __page_names file might contain the following 
 *   entries.
 *
 *     defaultHome|http://your-server-name/index.html|text/html
 *     apacheManual|http://your-server-name/manual/index.html|text/html
 *     latestSoftware|http://your-server-name/software.tar.gz|application/tar
 *
 *   To browse the server's manual, use a query like the following 
 *   (assuming that the pageName program is in the cgi-bin directory).
 *
 *     http://your-web-server/cgi-bin/pageName?page=apacheManual
 *
 *   The program should be able to handle most MIME types if the browser
 *   knows about them, like the third entry in the __page_names example 
 *   above.
 *
 *   Installation:
 *   1. Build the pageName program.
 *
 *      ctcc -I . pageName.c -o pageName
 *
 *      You must include the current directory in the Ctalk
 *      search path (with -I .), so Ctalk can locate the CGIApp 
 *      class library.
 *
 *   2. Copy the pageNam binary to the server's CGI directory,
 *      and set its permissions. If you don't know where the
 *      CGI directory is, look at the CGI configuration in the 
 *      server's httpd.conf file. For example, for a normal 
 *      Apache 2 installation:
 *
 *      $ su
 *      # cp pageName /usr/local/apache2/cgi-bin
 *      # chmod 0755 /usr/local/apache2/cgi-bin/pageName
 *
 *      If necessary, change the owner and group of 
 *      the pageName binary so the server can execute it.
 *
 *   3. Perform any additional configuration the 
 *      server needs.  In addition to configuring 
 *      the server to run CGI scripts, you might need
 *      to add the ctalk libraries to the server's 
 *      library path, with a httpd.conf line similar to the 
 *      following, outside of any directory wrapper.
 *
 *      SetEnv LD_LIBRARY_PATH /usr/local/lib
 */

#define PAGEFILENAME "__page_names"

int main (int argc, char **argv, char **envp) {

  CGIApp new pageNameApp;
  String new pageRecordPath;
  String new pageRecord;
  String new pageQuery;
  ReadFileStream new pageRecordStream;
  String new queryValueString;
  String new pageNameString;
  String new urlString;
  String new contentTypeString;
  Array new pageRecordFields;
  Integer new nRecordFields;
  Integer new i;
  Exception new e;

  pageNameApp getServerEnv envp;

  pageRecordPath = 
    pageNameApp serverEnvironment at "DOCUMENT_ROOT";
  pageRecordPath += "/";
  pageRecordPath += PAGEFILENAME;

  pageRecordStream openOn pageRecordPath;
  if (e pending) {
    pageNameApp httpHeader;
    pageNameApp htmlPageHeader "pageName Error";
    pageNameApp outputString "<body>\n";
    pageNameApp outputString "<strong>pageName Error:</strong> ";
    pageNameApp outputString "Error reading ";
    pageNameApp outputString pageRecordPath;
    e handle;
    exit (1);
  }

  /*
   *  parseQueryString stores the value of the page=<pagename>
   *  query as a String. AssociativeArray method at retrieves 
   *  the String object.
   */
  pageNameApp parseQueryString;
  queryValueString = pageNameApp queryValues at "page";

  while (!pageRecordStream streamEof) {
    pageRecord = pageRecordStream readLine;
    /*
     *  When using split, make sure that the first argument
     *  is a Character, and that it is unlikely to be duplicated
     *  unnecessarily in the receiver String - otherwise split
     *  may return more elements than you intended.
     */
    nRecordFields = pageRecord split '|' , pageRecordFields;
    if (nRecordFields != 3) {
      break;
    }
    /*
     *  Apps still need to assign array elements to
     *  an object of the value's class in some cases.  
     *  
     *  Here, the class of the pageRecordField's values is
     *  String.
     */
    pageNameString = pageRecordFields at 0;
    urlString = pageRecordFields at 1;
    contentTypeString = pageRecordFields at 2;
    if (pageNameString == queryValueString) {
      pageNameApp contentLocationHeader contentTypeString, urlString;
      return 0;
    }
  }

  /*
   *  If the program hasn't found the page, print an 
   *  error message.
   */
  pageNameApp httpHeader;
  pageNameApp htmlPageHeader "pageName Error";
  pageNameApp outputString "<strong>pageName Error:</strong> Page <i>";
  pageNameApp outputString queryValueString;
  pageNameApp outputString "</i> not found.";
  pageNameApp htmlPageFooter;

  return 0;
}
