# $Id: configure.ac,v 1.3 2020/10/17 11:02:03 rkiesling Exp $
AC_INIT(ctalk, 0.0.66, rk3314042@gmail.com, "")

# Contained in acinclude.m4 and generated aclocal.m4
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL

AC_CANONICAL_HOST
AM_INIT_AUTOMAKE

# The --without-x command line option; sets $no_x.  Also checks
# for the --x-includes (sets $x_includes) and --x-libraries
# (sets $x_libraries) options.
AC_PATH_X

ct_x_cflags=""
if test "x$no_x" != "xyes" ; then
   # if the user didn't give a --x-includes option, then the
   # internal $x_includes variable is blank. We still need the
   # include path, so check some locations ourselves.
   if test "x$x_includes" != "x"; then 
     ct_x_cflags="-I $x_includes"
     x_cflags="-I $x_includes"
     AS_ECHO_N "$x_includes" >build/.x_h_path
   elif test -f /usr/X11R6/include/X11/X.h; then
     ct_x_cflags="-I /usr/X11R6/include"
     x_cflags="-I /usr/X11R6/include"
     AS_ECHO_N "/usr/X11R6/include" >build/.x_h_path
   elif test -f /usr/X11/include/X11/X.h; then
     ct_x_cflags="-I /usr/X11/include"
     x_cflags="-I /usr/X11/include"
     AS_ECHO_N "/usr/X11/include" >build/.x_h_path
   elif test -f /usr/include/X11/X.h; then
     ct_x_cflags="-I /usr/include"
     x_cflags="-I /usr/include"
     AS_ECHO_N "/usr/include" >build/.x_h_path
   fi
fi

AC_DEFINE(ctdb_ctcc_ldflags)
ctdb_ctcc_ldflags=""
case $host_os in
     *darwin*)
# Note that we need to add this regardless of LD_LIBRARY_PATH,
# in case "make install's" shell doesn't have LD_LIBRARY_PATH
# in its environment.
     AS_ECHO_N "checking the Darwin X library path... "
     AS_ECHO $x_libraries
     if test "x$x_libraries" != "xNONE"; then
       ctdb_ctcc_ldflags="-L$x_libraries"
     fi
     ;;
esac     

AM_MAINTAINER_MODE
AC_CONFIG_HEADERS([config.h])

AH_TOP([#ifndef _CTALK_CONFIG_H])
AH_BOTTOM([#define _CTALK_CONFIG_H
#endif /* _CTALK_CONFIG_H */])

AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(PPVERSION)
PACKAGE=ctalk
VERSION=0.0.66

AC_SUBST(LIB_AM_LDFLAGS)
AC_SUBST(AM_CFLAGS)
AC_SUBST(LIB_MAJOR_VERSION)
AC_SUBST(LIB_MINOR_VERSION)

LIB_MAJOR_VERSION=0
LIB_MINOR_VERSION=66

AC_PROG_CC
AC_PROG_YACC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_DEFINE(def_cflags)
AC_DEFINE(def_optflags)
AC_DEFINE(def_ctalkflags)
AC_DEFINE(cc_wno_long_double)

def_cflags="-g"
ctdb_ctcc_ldflags="$ctdb_ctcc_ldflags -g"
if test $prefix = "NONE" ; then
  AS_ECHO_N "/usr/local/include/ctalk" > build/.includedir
else
  AS_ECHO_N "$prefix/include/ctalk" > build/.includedir
fi


AC_PATH_PROG(makeinfo_path, makeinfo,)

if test "x$ac_cv_path_makeinfo_path" = "x"; then
AS_ECHO -------------------------------------------------------------
AS_ECHO "Configure did not find makeinfo, part of the GNU texinfo package."
AS_ECHO "It is required to build the Ctalk Texinfo documentation and is"
AS_ECHO "available at http://www.gnu.org/software/texinfo/."
AS_ECHO -------------------------------------------------------------
exit 1
fi

AS_ECHO_N "checking the installation user's name... "
if test -n $LOGNAME; then 
   AS_ECHO  $LOGNAME > build/.confname
else if test -n $EUID; then
   AS_ECHO $EUID > build/.confname;
fi
fi
cat build/.confname

AS_ECHO_N "checking the installation user's group... "
./build/inst_group.sh > build/.confgroup
cat build/.confgroup

AS_ECHO_N "checking the installation user's HOME directory... "
if test -n $HOME; then
   AS_ECHO $HOME > build/.confuserhomedir
fi
cat build/.confuserhomedir

case $host_os in
     *cygwin*) lib_ext=.a ;;
     *djgpp* ) lib_ext=.a ;;
     *darwin* ) 
	     if test "$enable_shared" = no || test "$enable_static" = yes ;then
		lib_ext=.a
	     else
		lib_ext=.dylib 
	     fi
	     ;;
     *       ) 
	     if test "$enable_shared" = no || test "$enable_static" = yes ;then
		lib_ext=.a
	     else
		lib_ext=.so 
	     fi
	     ;;
esac
AC_SUBST(lib_ext)

AC_DEFINE(use_pthreads)
AC_SUBST(use_pthreads)
AS_ECHO_N "checking if the compiler knows about -pthreads... "
./build/pthread.sh

if test -f build/.pthread_gcc_option </dev/null 2>&1 ; then
  use_pthreads="-pthread"
  AS_ECHO "yes"
else
  use_pthreads=""
  AS_ECHO "no"
fi

user_glut_opt=""
AC_ARG_WITH(glut,
  [  --with-glut=OPT         Compile and link with GLUT and OpenGL
                          support.  If OPT is [[no]], do not include
                          GLUT support.  This also overrides OS X
                          support for the GLUT application framework.
                          [[yes]]], 
  [  case "$withval" in
     yes)
        user_glut_opt="yes"
        ;;
     no)
        user_glut_opt="no"
        ;;
     esac
  ]
)
AC_SUBST(user_glut_opt)

AC_ARG_WITH(pthreads,
  [  --with-pthreads=OPT     Compile and link using the system's 
                          pthreads library. [[yes]]],
  [    case "$withval" in
       no)
	   use_pthreads=""
       ;;
     esac
  ]
)
def_cflags=$def_cflags" $use_pthreads"

AS_ECHO_N "checking if the compiler undertands -Wno-pointer-to-int-cast... "
$CC -Wpointer-to-int-cast build/conftest.c -o build/conftest >/dev/null 2>&1
accret=$?
if test $accret = "0"; then
AS_ECHO "yes"
def_cflags=$def_cflags" -Wno-pointer-to-int-cast "
touch build/.cttest_no_pointer_to_int_cast
else
AS_ECHO "no"
fi

AC_SUBST(no_restrict_warning)
AS_ECHO_N "checking if the compiler undertands -Wrestrict... "
$CC -Wrestrict build/conftest.c -o build/conftest >/dev/null 2>&1
accret=$?
if test $accret = "0"; then
AS_ECHO "yes"
no_restrict_warning="-Wno-restrict "
else
AS_ECHO "no"
no_restrict_warning=" "
fi

AC_SUBST(no_stringop_overflow_warning)
AS_ECHO_N "checking if the compiler undertands -Wno-stringop-overflow... "
$CC -Wno-stringop-overflow build/conftest.c -o build/conftest >/dev/null 2>&1
accret=$?
if test $accret = "0"; then
AS_ECHO "yes"
no_stringop_overflow_warning="-Wno-stringop-overflow "
touch build/.cttest_no_stringop_overflow
else
AS_ECHO "no"
no_stringop_overflow_warning=" "
fi

AC_SUBST(stringop_truncation_warning)
AS_ECHO_N "checking if the compiler undertands -Wstringop-truncation... "
$CC -Wstringop-truncation build/conftest.c -o build/conftest >/dev/null 2>&1
accret=$?
if test $accret = "0"; then
AS_ECHO "yes"
stringop_truncation_warning="-Wno-stringop-truncation "
else
AS_ECHO "no"
stringop_truncation_warning=" "
fi
def_cflags=$def_cflags" $stringop_truncation_warning"

AC_SUBST(address_of_packed_member_warning)
AS_ECHO_N "checking if the compiler undertands -Waddress-of-packed-member... "
$CC -Waddress-of-packed-member build/conftest.c -o build/conftest >/dev/null 2>&1
accret=$?
if test $accret = "0"; then
AS_ECHO "yes"
address_of_packed_member_warning="-Wno-address-of-packed-member "
touch build/.cttest_no_address_of_packed_member
else
AS_ECHO "no"
address_of_packed_member_warning=" "
fi
def_cflags=$def_cflags" $address_of_packed_member_warning"

AC_SUBST(no_unknown_warning_warning)
AS_ECHO_N "checking whether to add -Wno-unknown-warning-option for LLVM... "
./build/have_llvm.sh
if test -f build/.llvm; then
  AS_ECHO "yes"
  no_unknown_warning_warning="-Wno-unknown-warning-option "
  def_cflags=$def_cflags" -Wno-unknown-warning-option"
else
  AS_ECHO "no"
  no_unknown_warning_warning=" "
fi  


AC_DEFINE(gnuc_version)
AC_SUBST(gnuc_version)
AC_ARG_WITH(gnuc_version,
  [  --with-gnuc-version     Try to figure out the version of 
                          GCC and define __GNUC__, __GNUC_MINOR__,
                          and __GNUC_PATCHLEVEL__, regardless 
                          of the host GCC's definitions.  
                          If not given, only define these macros 
                          if the host GCC defines them.],
  [
    gnuc_version="-DCTPP_GNUC_VERSION"
  ],
    gnuc_version=""
)

AC_DEFINE(CONF_INCLUDE_PATH, "")
AC_SUBST(CONF_INCLUDE_PATH)
have_conf_include_path="no"
AC_ARG_WITH(include_path, 
  [  --with-include-path=DIRS 
                          Use the header files located 
                          at DIRS. The argument is a colon
                          separated list of directories; e.g., 
                          <dir1>:<dir2>:<dir3> ...],
  [
    CONF_INCLUDE_PATH="-DCONF_INCLUDE_PATH=\\\"$withval\\\""
    have_conf_include_path="yes"
    ],
    CONF_INCLUDE_PATH=""
    have_conf_include_path="no"
)

make_generic_path="no"
AC_SUBST(generic_path)
AC_ARG_WITH(generic_path,
  [  --with-generic-path=[[yes/no]]
                          If yes, use only /usr/include and 
                          /usr/local/include to search for include 
                          files. [[no]]],
  [generic_path=$withval
    case "$withval" in
    yes)
      generic_path="-DGENERIC_PATHS"
      make_generic_path="yes"
      ;;
    no)
      generic_path=""
      make_generic_path="no"
      ;;
    *)
      generic_path="-DGENERIC_PATHS"
      make_generic_path="yes"
      ;;
    esac
  ],
  [
      make_generic_path="no"
  ]
)

make_mit_posix_path="no"
AC_SUBST(def_mit_posix_path)
AC_ARG_WITH(mit_posix_path,
  [  --with-mit-posix-path=[[yes/no]]
                          If yes, use an include search path that 
                          conforms to a generic GCC directory structure.  
                          See the README file for a description. [[no]]],
  [make_mit_posix_path=$withval
     case "$withval" in
     yes)
       make_mit_posix_path="yes"
       mit_posix_path_def="-DMIT_POSIX_PATH"
       ;;
     no)
       make_mit_posix_path="no"
       mit_posix_path_def="-DMIT_POSIX_PATH"
       ;;
     *)
       make_mit_posix_path="no"
       mit_posix_path_def="-DMIT_POSIX_PATH"
       ;;
    esac
  ],
  [
       make_mit_posix_path="no"
  ]
)

AC_ARG_WITH(inline-functions,
  [  --without-inline-functions  
                          Do not optimize by inlining functions.],
    [    case "$withval" in
	 no)
	   inline_function_opt=""
	   ;;
	 * )
	   inline_function_opt="-finline-functions"
	   ;;
	esac
	   ],
        inline_function_opt="-finline-functions"
)

#
#  From ctpp's configure.in. Ctpp still uses GCC_LIBDIR and the 
#  -isystem option if necessary.
#
AC_ARG_WITH(gcc_includes, 
  [  --with-gcc-includes[=path]  Use the gcc library header files located 
                            in <path>. The argument can be yes|no|path.  
                            Without an argument or if the argument is yes 
                            (the default), try to determine the compiler 
                            configuration at run time. If no, configure
                            the preprocessor not to search for the GCC 
                            library include files.],
  [    case "$withval" in
         yes)
	   use_gcc_includes="-DUSE_GCC_INCLUDES"
	   ;;
	 no)
	   use_gcc_includes="-DNO_GCC_INCLUDES"
	   ;;
	 * )
	   use_gcc_includes="-DUSE_GCC_INCLUDES"
	   if [ test -d "$withval" ] ; then
	      gcc_include_path="-DGCC_INCLUDE_PATH=\\\"$withval\\\""
           fi
	   ;;
	esac
	   ],
        use_gcc_includes="-DUSE_GCC_INCLUDES"
)

case $host_os in
 	  *darwin*)  # libincdir is set below.
	  AS_ECHO_N "checking Darwin builtin functions (this may take a few minutes)... ";
	  `./build/osxbuiltins.sh`;
	  AS_ECHO "ok"
	  ;;	  
	  *)
	  AS_ECHO_N "checking the compiler's include directory... ";
	  libincdir=`./build/gccincdir.sh`;
	  if [ ! test -d "$libincdir" ] ; then
	  AS_ECHO "not found";
	  AC_MSG_WARN(Could not locate the GCC library include files.)
	  AC_MSG_WARN(Use the --with-gcc-includes configure option, or)
	  AC_MSG_WARN(consult the README file in this directory for)
	  AC_MSG_WARN(instructions for this system.)
	  else
	  AS_ECHO $libincdir;
	  fi
          ;;
esac
CFLAGS=""

AC_ARG_WITH(long-double,
  [ --with-long-double[=y/n]      Either yes or no if the compiler supports
                             a long double warning.  If this option isn't
                             given, then the configuration tries to 
			     determine it. Normally only needed for
                             PowerPC systems that issue warnings that
                             the system doesn't support long doubles.],
  [   case "$withval" in
        [yY][eE][sS])
           user_long_double="yes"
           ;;
	*)
           user_long_double="no"
           ;;
      esac
         ],
)


freetype_includes=""
freetype_headers=""
have_freetype2="no"
user_without_freetype_opt=""
if test "x$no_x" != "xyes" ; then
AC_ARG_WITH(freetype,
[ --without-freetype       Don't use freetype2 even if it is
                          present.  This is separate from the
			  freetype2 library support in libXft.],
     [
        case "$withval" in
	  [Nn][oO])
	  user_without_freetype_opt="no"
	  ;;
	  *)
	  user_without_freetype_opt="yes"
	  ;;
	esac
     ]
)     
	  
if test "x$user_without_freetype_opt" != "xyes"; then
AC_ARG_WITH(freetype-includes,
 [ --with-freetype-includes[=DIR]
                          The directory where the main freetype include
			  file, ft2build.h, is located.],
 [   case "$withval" in
  	*)
	   if test -f "$withval/ft2build.h"; then
	     freetype_includes="-DHAVE_FREETYPE_H -I $withval";
	     have_freetype2="yes"
	   else
	     freetype_includes="-DHAVE_FREETYPE_H -I $withval";
	     have_freetype2="yes"
	     AC_MSG_ERROR(--with-freetype-includes=$withval:
$withval/ft2build.h: file not found., [1]);
	   fi
           ;;
     esac
   ]
)
fi

if test "$have_freetype2" != "yes" && \
   	test "x$user_without_freetype_opt" != "xyes"; then
   if test -f "/usr/include/freetype2/ft2build.h"; then
     freetype_includes="-DHAVE_FREETYPE_H -I /usr/include/freetype2"
     have_freetype2="yes"
   elif test -f "/usr/local/include/freetype2/ft2build.h"; then
     freetype_includes="-DHAVE_FREETYPE_H -I /usr/local/include/freetype2"
     have_freetype2="yes"
   elif test -f "/usr/X11/include/freetype2/ft2build.h"; then
    freetype_includes="-DHAVE_FREETYPE_H -I /usr/X11/include/freetype2/freetype"
    have_freetype2="yes"
  fi
fi

fi # if test no_x != "yes"


AS_ECHO_N "checking if the compiler recognizes -Wno-long-double... "
./build/gcccaps.sh

if test -f build/.gccnolongdouble ; then
   AS_ECHO no
elif test -f build/.gcchavelongdouble; then
   AS_ECHO yes
else
   AS_ECHO "There was an error in build/gcccaps.sh script.  Please check"
   AS_ECHO "your system before trying to build Ctalk."
   exit 1
fi

AS_ECHO_N "checking if the compiler recognizes -framework... "
if test -f build/.gccnoframeworkopt; then
   AS_ECHO no
elif test -f build/.gcchaveframeworkopt; then
   AS_ECHO yes
fi

AS_ECHO_N "checking if the compiler recognizes -Wno-format-overflow... "
if test -f build/.gccdonthavenoformatoverflowopt; then
   AS_ECHO no;
elif test -f build/.gcchavenoformatoverflowopt; then
   AS_ECHO yes;
   def_cflags=$def_cflags" -Wno-format-overflow"
   touch build/.cttest_no_format_overflow
fi

if test "$user_long_double"+yes != "yes";  then
     if test "$user_long_double" = "yes"; then
       cc_wno_long_double="-Wno-long-double"
     else
       cc_wno_long_double=""
     fi
else 
     if test -f build/.gcchavelongdouble; then
        cc_wno_long_double="-Wno-long-double"
     else
	cc_wno_long_double=""
     fi
fi

case "$build" in
     *-pc-cygwin)
     	   parser_opt_flags="-g -O2"
	   lib_opt_flags="-g -O1"
	   def_optflags=$lib_opt_flags
	   lib_rt_flags=""
	   ;;
     *-pc-linux-gnu)
     	   parser_opt_flags="-g -O3"
	   lib_opt_flags="-g -O3"
	   def_optflags=$lib_opt_flags
	   lib_rt_flags=""
	   ;;
     *sparc* ) 
     	   parser_opt_flags="-g -O2"
	   lib_opt_flags="-g"
	   lib_rt_flags="-lrt"
	   ctdb_ctcc_ldflags=$ctdb_ctcc_ldflags" $lib_rt_flags"
	   ;;
     *darwin*)
           def_cflags=$def_cflags" $cc_wno_long_double"
           parser_opt_flags="-g -O3 "
           lib_opt_flags="-g -O3 "
	   lib_rt_flags=""
           ;;
     * ) 
           parser_opt_flags=""
           lib_opt_flags="-g"
	   lib_rt_flags=""
           ;;
esac

#
# --without-optimization is basically a euphemism for, "debug version,"
# so maybe someday we'll add a --with-debugging synonym to the 
# ./configure options.  For now, just add -DDEBUG_MESSAGES to the
# preprocessor defines.
# 
# Note - the DEBUG_MESSAGES define is not used very much and not
# very useful to anyone but me, and should just be defined in a 
# source file or in ctalk.h.
#
AC_ARG_WITH(optimization,
  [  --without-optimization  
                          Do not use -O<n> compiler optimization.
                          This is the default for GCC versions earlier
                          than 4.0.
                          [[with optimization]].],
    [    case "$withval" in

	 no)
#	   AS_ECHO checking whether to enable debugging messages... yes
#	   parser_opt_flags="-DDEBUG_MESSAGES"
#	   lib_opt_flags="-g -DDEBUG_MESSAGES"
	   parser_opt_flags="-g"
	   lib_opt_flags="-g"
	   def_optflags=$lib_opt_flags
	   ;;
	esac
	   ]
)

AC_SUBST(no_fmt_trunc_warning)
no_fmt_trunc_warning=""
AC_SUBST(no_strict_overflow_warning)
no_strict_overflow_warning=""
AS_ECHO_N "checking version specific compiler options... "
case `./build/gccmajorver.sh` in
     2|3)
     AS_ECHO "version 3 or earlier"
     parser_opt_flags="-g "
     lib_opt_flags="-g "
     ;;
     7)
     AS_ECHO "version 7"
     no_fmt_trunc_warning="-Wno-format-truncation -Wno-format-overflow"
     no_strict_overflow_warning="-Wno-strict-overflow "
     ;;
     *)
     AS_ECHO "none needed"
     ;;
esac

AC_ARG_WITH(profiling,
  [  --with-profiling        Compile with profiling information for gprof.],
    [  case "$withval" in
       yes)
          parser_opt_flags="-pg -g"
          lib_opt_flags="-pg -g"
	  def_cflags=$def_cflags" -pg"
	  ctdb_ctcc_ldflags=$ctdb_ctcc_ldflags" -pg"
          ;;
       esac
    ]
)

# The built-in --without-x option is checked by AC_PATH_X, above.
#
# AC_ARG_WITH(x_user_opt,
#  [  --without-x             Build without X Window System support.
#                          Overrides configure's built-in --with-x option.],
#    [  case "$withval" in
#       *)
#        ;;
#       esac
#    ]
#)

GNU_READLINE_ARG=""
AC_ARG_WITH(gnu-readline,
	[ --with-gnu-readline=DIR
                          Link GNU readline libraries from DIR/lib, 
                          and include files from DIR/include.  If 
                          DIR is, "no," then build Ctalk without 
                          GNU readline.  The default is to link Ctalk 
                          with the GNU readline libraries if they are 
                          located either in /usr or /usr/local.],
        [
	GNU_READLINE_ARG="$withval"
        ]
)

have_xft="no"
no_xft_user_opt="no"

LIB_FREETYPE_INCLUDE_ARG=""
if test "x$no_x" = "x"; then
AC_ARG_WITH(libxft-includes,
	[ --with-libxft-includes=DIR
                          The directory where X11/Xft/Xft.h is located
			  when building Ctalk with Xft support. ],
        [
        if test "x$withval" != "xno" && test "x$withval" != "xNO"; then
	  if test -f "$withval/X11/Xft/Xft.h"; then
	    LIB_FREETYPE_INCLUDE_ARG="$withval";
            if test "x$withval" != "xno" || test "x$withval" != "xNO"; then
              have_xft="yes"
	    fi
	  else
 	    AC_MSG_ERROR(--with-xft-includes=$withval:
$withval/X11/Xft/Xft.h: file not found., [1]);
	  fi
	else
	no_xft_user_opt="yes"
	fi
        ]
)
fi # if test "x$no_x" = "x"; then

LIB_FREETYPE_LIB_ARG=""
if test "x$no_x" = "x"; then
AC_ARG_WITH(libxft-libs,
	[ --with-libxft-libs=DIR
                          Look for the X FreeType font libraries (libXft)
                          in DIR. If DIR = 'no', then don't include
			  '-L<DIR> -lXft' in the compilation flags],
        [
	case "$withval" in
	no|NO)
	no_xft_user_opt="yes"
	;;
	*)
	LIB_FREETYPE_LIB_ARG="$withval"
	;;
	esac
        ]
)
fi # if test "x$no_x" = "x"; then

lib_freetype_cflags=""
have_xrender="no"
XRENDER_INC_ARG=""
if test "x$no_x" = "x"; then
AC_ARG_WITH(xrender-includes,
	[ --with-xrender-includes=DIR
                          Look for Xrender.h and the other
			  X Render extension include files in
			  <DIR>/X11/extensions. If DIR is
			  "no" build Ctalk without X Render support.
			  Because X Render is a server-side module,
			  no separate libraries are needed for the
			  needed for client programs.  Note that
			  drawing with the XRender/Freetype/Xft
			  libraries is new and relatively untested,
			  and should be considered experimental.],
        [
	XRENDER_INC_ARG="$withval"
        if test "x$withval" != "xno" && test "x$withval" != "xNO"; then
	  if test -f $withval/X11/extensions/Xrender.h; then
	    have_xrender="yes";
	    lib_freetype_cflags="-I $withval -DHAVE_XRENDER_H";
	  else
 	    AC_MSG_ERROR(--with-xrender-includes=$withval:
$withval/X11/extensions/Xrender.h: file not found., [1]);
	  fi
	fi
        ]
)
fi # if test "x$no_x" = "x"; then

if test "$no_x" != yes ; then
  case $host_os in
     *darwin*)
     x_cflags="$x_cflags -I/usr/include/X11 -I/usr/X11R6/include"
     x_cflags="$x_cflags $cc_no_long_double"
     if test "x$x_libraries" != "x" ; then
       x_ldflags="-L$x_libraries -L/usr/lib -L/usr/X11R6/lib -lX11 -lXext"
     else
       x_ldflags="-L/usr/lib -L/usr/X11R6/lib -lX11 -lXext"
     fi
     ;;
     *)
     x_cflags="-I/usr/include/X11 -I/usr/openwin/include -I/usr/X11R6/include"
     if test "x$x_libraries" != "x" ; then
       x_ldflags="-L$x_libraries -L/usr/lib -L/usr/X11R6/lib -lX11 -lXext"
     else
       x_ldflags="-L/usr/lib -L/usr/X11R6/lib -lX11 -lXext"
     fi
     ;;
  esac     
  AC_DEFINE([HAVE_X_H], [1], 
  [Define as 1 if the system has the X header files.])
  AC_SUBST(HAVE_X_H)			 
else 
  AC_DEFINE([HAVE_X_H], [0], 
  [Define as 1 if the system has the X header files.])
  AC_SUBST(HAVE_X_H)			 
fi


if test "$no_x" == "yes" ; then
   no_x11_def="-DWITHOUT_X11"
   touch build/.no_x_support
else
   no_x11_def=""
fi

x_protocol_errors="-DWITHOUT_X_PROTOCOL_ERRORS"
AC_ARG_WITH(x_protocol_errors,
  [  --with-x-protocol-errors        
                          Print X11 protocol error messages. This is
			  sometimes useful for debugging.],
    [case "$withval" in
       *)
          x_protocol_errors="-DWITHOUT_X_PROTOCOL_ERRORS"
          ;;
        yes)
          x_protocol_errors=""
          ;;
       esac
    ]
)

AC_ARG_WITH(cleanup_errors,
  [  --without-cleanup-errors        
                          Don't print errors during object cleanup when
                          a Ctalk program exits.  Normally this indicates a
                          severely corrupted object, which causes the
                          the program to exit immediately.],
    [  case "$withval" in
       no)
          without_cleanup_errors_def="-DWITHOUT_CLEANUP_ERRORS"
          ;;
        *)
          without_cleanup_errors_def=""
          ;;
       esac
    ]
)

x11_warnings="-DWITHOUT_X11_WARNINGS"
AC_ARG_WITH(x11_warnings,
  [  --with-x11-warnings
                          Print warning messages from Ctalk's X11 
                          libraries.  Because most internal errors are 
                          recoverable, this option is mainly useful
                          for debugging.],
    [  case "$withval" in
       *)
          x11_warnings="-DWITHOUT_X11_WARNINGS"
          ;;
        yes)
          without_x11_warnings_def=""
          ;;
       esac
    ]
)

AC_SUBST(without_signed_hex_octal_constant_warnings_def)
without_signed_hex_octal_constant_warnings_def="-DWITHOUT_SIGNED_HEX_OCTAL_CONSTANT_WARNINGS"

#
#  Note that since the option begins with "--without" the default is, "no."
#
AC_ARG_WITH(signed-hex-octal-constant-warnings,
  [  --without-signed-hex-octal-constant-warnings
                          Don't print warning messages when encountering
                          a signed hexadecimal or octal constant.[[no]]
			  Note that because the option begins with,
			  "--without-," the default is [[no]].
			  So to enable the warnings, use,
			  "--with-signed-hex-octal-constant-warnings."],
    [  case "$withval" in
       no)
          without_signed_hex_octal_constant_warnings_def="-DWITHOUT_SIGNED_HEX_OCTAL_CONSTANT_WARNINGS"
          ;;
       yes)
       without_signed_hex_octal_constant_warnings_def=""
       ;;
       *)
        without_signed_hex_octal_constant_warnings_def="-DWITHOUT_SIGNED_HEX_OCTAL_CONSTANT_WARNINGS"
          ;;
       esac
    ]
)

case $host_os in
     *darwin* ) 
          gcc_include_path="-DGCC_INCLUDE_PATH=\\\"/usr/include/gcc/darwin/default\\\""
     ;;
     *)
          gcc_include_path="-DGCC_INCLUDE_PATH=\\\"$libincdir\\\""
     ;;
esac

no_builtins=""
use_old_builtins=""
AC_ARG_WITH(builtins,
  [  --with-builtins=[yes/no/old] 
                            If no, build the preprocessor without built-in 
                            macros.  If old, then use one of the predefined 
                            builtin definitions in ctpp/config.  Anything else 
                            causes ctpp use the built-in macros that cpp 
                            lists with the command, 'AS_ECHO ' ' | cpp -dM -'.],
  [use_builtins=$withval
    case "$withval" in
    no)
      no_builtins="-DNO_BUILTINS"
      use_old_builtins=""
      ;;
    old)
      no_builtins=""
      use_old_builtins="-DUSE_OLD_BUILTINS"
      ;;
    esac
  ]
)

rm -f build/.nopreload
AC_ARG_WITH(preload-installation,
  [  --without-preload-installation
                            Install Ctalk without preloading any of the
			    base classes.  This might be necessary if 
			    the installation cannot determine the cache
			    directory or file permissions.  If you
			    give this option, you can still use Ctalk,
			    but it will run more slowly the first few
			    times you compile programs.], [
    touch build/.nopreload			    
]
)

rm -f build/.nolangtools
AC_ARG_WITH(language-tools,
  [  --without-language-tools
                            Skip building and installing the the language
                            tools when installing Ctalk.], [
    touch build/.nolangtools
]
)


without_circular_object_warnings="-DNO_CIRCULAR_OBJECT_WARNINGS"
AC_ARG_WITH(circular-object-reference-warnings,
  [  --with-circular-object-reference-warnings=yes
                            If yes, print warnings about circular object
                            references. [[Don't print warnings.]]],
  [   case "$withval" in
      yes)
           without_circular_object_warnings=""
      ;;
      esac
]
)
                   


AC_CHECK_FUNCS(atoll, 
		      [have_atoll_def="-DHAVE_ATOLL"],
		      [have_atoll_def=""])

AC_CHECK_FUNCS(glob, 
		      [have_glob_def="-DHAVE_GLOB"],
		      [have_glob_def=""])

AC_CHECK_HEADER(stdbool.h,
	[have_stdbool_def="-DHAVE_STDBOOL_H"],
	[have_stdbool_def=""])
# ctpp expands have_stdbool_def in its Makefile.am's.
AC_SUBST(have_stdbool_def)

AC_CHECK_HEADER(glob.h,
	[have_glob_h_def="-DHAVE_GLOB_H"],
	[have_glob_h_def=""])
AC_SUBST(have_glob_h__def)

AC_CHECK_HEADER(sys/socket.h,
	[have_sys_socket_h_def="-DHAVE_SYS_SOCKET_H"],
	[have_sys_socket_h_def=""])
AC_CHECK_HEADER(sys/un.h,
	[have_sys_un_h_def="-DHAVE_SYS_UN_H"],
	[have_sys_un_h_def=""])

###
###
###  GL/GLX/GLEW headers and libraries
###
###
LIB_GL_INC_ARG=""
AC_ARG_WITH(gl-includes,
	[ --with-gl-includes=DIR
	                    Look for the OpenGL include files GL/gl.h,
			    GL/glx.h, and GL/glew.h in DIR. If DIR is
			    "no" then build Ctalk without OpenGL
			    support.],
	[ case "$withval" in
	  no|No|NO|nO)
	  have_gl_h_def=""
	  user_gl_h_dir="no"
	  ;;
	  *)
	  have_gl_h_def="-I$withval -DHAVE_GL_H"
	  user_gl_h_dir="$withval"
	  ;;
	  esac
	]
)	

if test "x$no_x" != "xyes"; then

AS_ECHO "Checking for GL include files..."
build/glh.sh $user_gl_h_dir

AS_ECHO_N "Checking for GL/gl.h... "
if test -f "build/.gl_h_path"; then
AS_ECHO yes
  have_gl_h_def="-I `cat build/.gl_h_path` -DHAVE_GL_H"
  gl_h_info="yes"
else
AS_ECHO no
  have_gl_h_def=""
  gl_h_info="no"
fi
AC_SUBST(have_gl_h_def)

AS_ECHO_N "Checking for GL/glx.h... "
if test -f "build/.glx_h_path"; then
AS_ECHO yes
  have_glx_h_def="-I `cat build/.glx_h_path` -DHAVE_GLX_H"
else
AS_ECHO no
  have_glx_h_def=""
fi
AC_SUBST(have_glx_h_def)

AS_ECHO_N "Checking for GL/glew.h... "
if test -f "build/.glew_h_path"; then
AS_ECHO yes
   have_glew_h_def="-I `cat build/.glew_h_path` -DHAVE_GLEW_H"
   glew_h_info="yes"
else
AS_ECHO no
   have_glew_h_def=""
   glew_h_info="no"
fi
AC_SUBST(have_glew_h_def)

else

have_gl_h_def=""
gl_h_info="no"
have_glx_h_def=""
have_glew_h_def=""

fi 

gl_ldflags=""
case "$host_os" in
     *darwin*)
     AS_ECHO_N "checking for the MacOS OpenGL framework... ";
     if test -f /System/Library/Frameworks/OpenGL.framework/Headers/gl.h >/dev/null 2>&1; then
       AS_ECHO "yes"
       gl_ldflags="-lGL -lGLU"
       have_gl_h_def="-DHAVE_GL_H"
     else
       AS_ECHO "no"
     fi
     ;;
     *)
     AC_CHECK_LIB(GL, glColor4f, gl_ldflags="-lGL -lGLU",
		 gl_ldflags="")
     ;;
esac

AC_ARG_WITH(gl-libs,
	[ --with-gl-libs=DIR
	                    Look for the OpenGL libraries in DIR. If
			    DIR is "no" then build Ctalk without OpenGL
			    support.],
	[ case "$withval" in
	no|NO|No|nO)
	gl_ldflags=""
	;;
	*)
	gl_ldflags="-L$withval -lGL -lGLU"
	;;
	esac
	]
)	
if test "x$no_x" = "xyes" || test "x$no_x" = "xYES"; then
LIB_GL_LIB_ARG=""
else
LIB_GL_LIB_ARG="$gl_ldflags"
fi
x_ldflags="$x_ldflags $gl_ldflags"

AC_ARG_WITH(glew-includes,
  [  --with-glew-includes=DIR
                            Directory where the machine's GL/glew.h file
     			    and the related include files are located.],
  [  case "$withval" in
     *)
        if test -f "$withval/GL/glew.h"; then
          have_glew_h_def="-I $withval -DHAVE_GLEW_H";
	else
          have_glew_h_def="";
 	  AC_MSG_ERROR(--with-glew-includes=$withval:
$withval/GL/glew.h: file not found., [1]);
	fi	
        ;;
     esac
  ]
)
if test "x$no_x" = "xyes" || test "x$no_x" = "xYES"; then
have_glew_h_def=""
fi

AC_CHECK_LIB(GLEW, __glewLinkProgram, glew_ldflags="-lGLEW",
		 glew_ldflags="")

AC_ARG_WITH(glew-libs,
  [  --with-glew-libs=DIR
                            Directory where the the GL Extension 
                            Wrangler library, libGLEW, is located.],
  [  case "$withval" in
     no|NO|No|nO)
     glew_ldflags = ""
     ;;
     *)
     glew_ldflags = "-I$withval -lGLEW"
       ;;
     esac
  ]
)
if test "x$no_x" = "xyes" || test "x$no_x" = "xYES"; then
have_glew_ldflags=""
fi

x_ldflags="$x_ldflags $glew_ldflags"

have_glut_h_def=""
case $host_os in
     *darwin*)
     # On OSX glut.h is contained in a framework.
     have_glut_h_def="-DHAVE_GLUT_H"
     ;;
     *)
     AC_CHECK_HEADER(GL/glut.h,
		[have_glut_h_def="-DHAVE_GLUT_H"],
		[have_glut_h_def=""])
     ;;
esac

if test "x$user_glut_opt" = "xno" || test "x$user_glut_opt" = "xNO"; then
   have_glut_h_def=""
fi
if test "x$no_x" = "xyes" || test "x$no_x" = "xYES"; then
   have_glut_h_def="";
   have_glx_h_def="";
   have_glew_h_def="";
fi
AC_SUBST(have_glut_h_def)

if test "x$have_glut_h_def" = "x-DHAVE_GLUT_H"; then
   GLUT_H_ARG="GL/glut.h"
   case $host_os in
     *darwin*)
     # On OSX we might need to type this manually to handle
     # the GLUT and OpenGL frameworks.
     ;;
     *)
       if test "x$glut_ldflags" = "x"; then
         glut_ldflags=" -lGL -lGLU -lglut "
       fi
     ;;
     esac
else
   GLUT_H_ARG="no"
fi

if test $have_stdbool_def = "-DHAVE_STDBOOL_H"; then
AC_DEFINE([HAVE_STDBOOL_H], [1], 
  [Define to 1 if the system has stdbool.h.])
AC_SUBST(HAVE_STDBOOL_H)
else
AC_DEFINE([HAVE_STDBOOL_H], [0], 
  [Define to 1 if the system has stdbool.h.])
AC_SUBST(HAVE_STDBOOL_H)
fi


case "$host_os" in
     *cygwin*)  have_off_t="" ;;
     *djgpp* )  have_off_t="" ;;
     *darwin* ) have_off_t="" ;;
     * )        have_off_t="-DHAVE_OFF_T" ;;
esac

AC_CHECK_FUNCS(localtime_r,
		[have_localtime_r_def="-DHAVE_LOCALTIME_R"],
		[have_localtime_r_def=""])

TOP_LEVEL_DIR=`pwd`

AS_ECHO_N "checking for gcc stdint.h wrappers..."
gcc_dumpversion=`gcc -dumpversion`
gcc_majorver=`sh -c "echo $gcc_dumpversion | cut -f 1 -d."`
gcc_minorver=`sh -c "echo $gcc_dumpversion | cut -f 2 -d."`

# AC_DEFINE([GCC_STDINT_WRAPPER], [],
#	[Tell the preprocessor to use newer stdint wrappers])
# AC_SUBST(GCC_STDINT_WRAPPER)
if expr $gcc_majorver = '4' >/dev/null 2>&1; then
   if expr $gcc_minorver = '5' >/dev/null 2>&1; then
      AS_ECHO "/* This is a machine generated file.  Please do not edit! */" \
        > $TOP_LEVEL_DIR/ctpp/config/gcc45_types.h;
      AS_ECHO "#include <stdio.h>" \
        >> $TOP_LEVEL_DIR/ctpp/config/gcc45_types.h;
      printf "static char *gcc45_types\x5b\x5d = {\n" \
        >> $TOP_LEVEL_DIR/ctpp/config/gcc45_types.h;
      AS_ECHO ' ' | cpp -dM - | grep '_TYPE__' | \
        sed -e 's/\#define[ 	]*\([a-zA-Z0-9_]*\)/\"\1\",/g' | \
	sed -e 's/\",[ 	]*/\", \"/g' | \
	sed -e 's/$/",/g' \
	  >> $TOP_LEVEL_DIR/ctpp/config/gcc45_types.h;
      AS_ECHO "NULL, NULL," \
	  >> $TOP_LEVEL_DIR/ctpp/config/gcc45_types.h;
      sh -c "AS_ECHO }\;" >>$TOP_LEVEL_DIR/ctpp/config/gcc45_types.h
      gcc_stdint_wrapper="-DGCC_STDINT_WRAPPER";
      AS_ECHO " yes";
   else
      gcc_stdint_wrapper="";
      AS_ECHO " no"   ;
   fi
else
   AS_ECHO " no"
   gcc_stdint_wrapper="";
fi

AC_PATH_PROG(ACLOCAL, aclocal, no)

AC_PATH_PROG(AUTOMAKE, automake, no)

AC_PATH_PROG(AUTOCONF, autoconf, no)

AC_PATH_PROG(RANLIB, ranlib, no)

AC_PATH_PROG(MAKEINFO, makeinfo, no)

have_gnu_readline=""
if test "x$GNU_READLINE_ARG" = "x"; then
   AC_CHECK_HEADER(readline/readline.h, [acl_gnu_readline="yes"],
   			                [acl_gnu_readline="no"])
   AC_CHECK_HEADER(curses.h, [acl_curses="yes"],
			     [acl_curses="no"])
   AC_CHECK_HEADER(ncurses.h, [acl_ncurses="yes"],
			      [acl_ncurses="no"])
   if test "x$acl_gnu_readline" = "xyes"; then
      have_gnu_readline="-DHAVE_GNU_READLINE"
      GNU_READLINE_ARG="default"
      case $host_os in
      *darwin*)
        gnu_readline_ld_opts="-lreadline"
      ;;
      *)
        gnu_readline_ld_opts="-lreadline -lhistory"
      ;;
      esac
      if test "x$acl_curses" = "xno" && test "x$acl_ncurses" = "xno"; then
         AC_MSG_WARN(The readline libraries may need the curses or ncurses)
         AC_MSG_WARN(libraries.  If the build doesn't succeed then try)
         AC_MSG_WARN(building Ctalk with the --have-gnu-readline=no option)
	 AC_MSG_WARN(or install the GNU ncurses package from)
	 AC_MSG_WARN(https://ftp.gnu.org/gnu/ncurses/.)
      elif test "x$acl_ncurses" = "xyes"; then
         # use ncurses if both curses and ncurses are present
         gnu_readline_ld_opts=$gnu_readline_ld_opts" -lncurses"
      elif test "x$acl_curses" = "xyes"; then
         gnu_readline_ld_opts=$gnu_readline_ld_opts" -lcurses"
      fi
   else
      have_gnu_readline=""
      gnu_readline_ld_opts=""
   fi
elif test "x$GNU_READLINE_ARG" != "xno"; then
   # warn if we don't have curses or ncurses
   AC_CHECK_HEADER(curses.h, [acl_curses="yes"],
			     [acl_curses="no"])
   AC_CHECK_HEADER(ncurses.h, [acl_ncurses="yes"],
			      [acl_ncurses="no"])
   if test "x$acl_curses" = "xno" && test "x$acl_ncurses" = "xno"; then
      AC_MSG_WARN(The readline libraries may need the curses or ncurses)
      AC_MSG_WARN(libraries.  If the build doesn't succeed then try)
      AC_MSG_WARN(building Ctalk with the --have-gnu-readline=no option)
      AC_MSG_WARN(or install the GNU ncurses package from)
      AC_MSG_WARN(https://ftp.gnu.org/gnu/ncurses/.)
   fi
else
   # print something
   GNU_READLINE_ARG="no"
fi   

LIB_AM_LDFLAGS="$AM_LDFLAGS $gnu_readline_ld_opts"

if test "x$no_xft_user_opt" != "xyes"; then
AS_ECHO_N "checking for FreeType includes... "
if test -z $LIB_FREETYPE_INCLUDE_ARG >/dev/null 2>&1; then
   if test -f /usr/local/include/X11/Xft/Xft.h >/dev/null 2>&1; then
      if test "x$no_x" = "x"; then
        have_xft="yes"
        LIB_FREETYPE_INCLUDE_ARG="/usr/local"
        lib_freetype_ld_opts="-lXft -lfreetype -lfontconfig"
      else
        LIB_FREETYPE_INCLUDE_ARG="no"
        lib_freetype_ld_opts=""
      fi
   elif test -f /usr/include/X11/Xft/Xft.h >/dev/null 2>&1; then
      if test "x$no_x" = "x"; then
        have_xft="yes"
        LIB_FREETYPE_INCLUDE_ARG="/usr"
        lib_freetype_ld_opts="-lXft -lfreetype -lfontconfig"
      else
        LIB_FREETYPE_INCLUDE_ARG="no"
        lib_freetype_ld_opts=""
      fi
   else
      LIB_FREETYPE_INCLUDE_ARG="no"
      lib_freetype_ld_opts=""
   fi
fi
AS_ECHO $LIB_FREETYPE_INCLUDE_ARG
if expr $LIB_FREETYPE_INCLUDE_ARG = "no" >/dev/null 2>&1; then
   freetype_cflags="";
   freetype_ld_opts="";
   AC_DEFINE([HAVE_XFT_H], 0, 
   [Set to 1 if Ctalk is built with libXft.])
else
   if test -f /usr/local/include/X11/Xft/Xft.h >/dev/null 2>&1; then
      have_xft="yes"
      LIB_FREETYPE_INCLUDE_ARG="/usr/local"
      AC_DEFINE([HAVE_XFT_H], 1, 
      [Set to 1 if Ctalk is built with libXft.])
      lib_freetype_cflags="$lib_freetype_cflags -DHAVE_XFT_H -I /usr/local/include/freetype2 -I /usr/local/X11R6/include/freetype2 "
   elif test -f /usr/include/X11/Xft/Xft.h >/dev/null 2>&1; then
      have_xft="yes"
      AC_DEFINE([HAVE_XFT_H], 1, 
      [Set to 1 if Ctalk is built with libXft.])
      LIB_FREETYPE_INCLUDE_ARG="/usr"
      lib_freetype_cflags="$lib_freetype_cflags -DHAVE_XFT_H -I /usr/local/include/freetype2 -I /usr/include/freetype2 -I /usr/X11R6/include/freetype2 "
   else
      AC_DEFINE([HAVE_XFT_H], 0, 
      [Set to 1 if Ctalk is built with libXft.])
      LIB_FREETYPE_INCLUDE_ARG="not found"
   fi
fi
AC_SUBST(HAVE_XFT_H)
fi # if test "x$no_xft_user_opt" != "xyes"; then

if test "x$LIB_FREETYPE_LIB_ARG" != "xno" || test "x$LIB_FREETYPE_LIB_ARG" != "xNO"; then
  AC_CHECK_LIB(Xft, XftDrawCreate, lib_freetype_ld_opts="-lXft -lfreetype -lfontconfig",
  		  lib_freetype_ld_opts=)
else 
  lib_freetype_ld_opts=
fi		  
AS_ECHO_N "checking for FreeType libs... "
if test -z $LIB_FREETYPE_LIB_ARG >/dev/null 2>&1; then
   if test "x$LIB_FREETYPE_LIB_ARG" != "xno" || test "x$LIB_FREETYPE_LIB_ARG" != "xNO"; then
     if test -z lib_freetype_ld_opts >/dev/null 2>&1; then
        AS_ECHO "no"
     else
        x_ldflags="$x_ldflags -lXft -lfreetype -lfontconfig"
        AS_ECHO "yes"
     fi
   else
     AS_ECHO "no"
   fi
else
   if test "x$LIB_FREETYPE_LIB_ARG" != "xno"; then
      lib_freetype_ld_opts=" -L"$LIB_FREETYPE_LIB_ARG" -lXft -lfreetype -lfontconfig"
      x_ldflags="$x_ldflags -L$LIB_FREETYPE_LIB_ARG -lXft -lfreetype -lfontconfig"
      AS_ECHO $LIB_FREETYPE_LIB_ARG
   else
      AS_ECHO "no"
   fi
fi

#
# Needs a better test if we decide to support old machines.
#
# if test "x$no_x" != "xyes"; then
# AC_CHECK_LIB(fontconfig, FcPatternFormat,
#			 [],
#			 [
# cat <<EOF
# This machine uses an old version of the fontconfig library.  You should
# consider installing a more recent version of fontconfig, which is available
# at: http://www.freedesktop.org/software/fontconfig/release/.  Alternatively,
# you can build Ctalk without X window support, by adding the --without-x
# option to configure.
# EOF
# exit 1])
# fi

AC_CHECK_LIB(GL, glWindowPos2i, glwindowpos2i_def="-DHAVE_GLWINDOWPOS",
		 glwindowpos2i_def="", "-L/usr/X11R6/lib")


if test "$no_x" != yes; then
  LIB_AM_LDFLAGS="$LIB_AM_LDFLAGS $lib_freetype_ld_opts"
fi  

AS_ECHO_N "checking for Xrender.h... "
if test "x$no_x" = "x"; then
   if test -z $XRENDER_INC_ARG >/dev/null 2>&1; then
    if test -f /usr/include/X11/extensions/Xrender.h >/dev/null 2>&1; then
      have_xrender="yes"
      LIB_XRENDER_ARG="Xrender.h"
      AC_DEFINE([HAVE_XRENDER_H], 1, 
       [Set to 1 if the system has Xrender.h.])
       lib_freetype_cflags="-DHAVE_XRENDER_H $lib_freetype_cflags"
       x_ldflags="$x_ldflags -lXrender -lm"
    elif test -f /usr/local/include/X11/Xft/Xft.h >/dev/null 2>&1; then
      have_xrender="yes"
      LIB_XRENDER_ARG="Xrender.h"
      AC_DEFINE([HAVE_XRENDER_H], 1, 
      [Set to 1 if the system has Xrender.h.])
      lib_freetype_cflags="-DHAVE_XRENDER_H $lib_freetype_cflags"
       x_ldflags="$x_ldflags -lXrender -lm"
    else
      LIB_XRENDER_ARG="not found"
      AC_DEFINE([HAVE_XRENDER_H], 0, 
      [Set to 1 if the system has Xrender.h.])
    fi # if test -f /usr/include/X11/extensions/Xrender.h...
   else # if test -z $XRENDER_INC_ARG...
    if test "x$XRENDER_INC_ARG" = "xno" || test "x$XRENDER_INC_ARG" = "xNO"; then
      LIB_XRENDER_ARG="no"
      AC_DEFINE([HAVE_XRENDER_H], 0, 
      [Set to 1 if the system has Xrender.h.])
       x_ldflags="$x_ldflags -lm"
    else
      have_xrender="yes"
      LIB_XRENDER_ARG="Xrender.h"
      AC_DEFINE([HAVE_XRENDER_H], 1, 
      [Set to 1 if the system has Xrender.h.])
       x_ldflags="$x_ldflags -lXrender -lm"
    fi
   fi # if test -z $XRENDER_INC_ARG...
else # if test "x$no_x" = "x"; then
  LIB_XRENDER_ARG="no"
 AC_DEFINE([HAVE_XRENDER_H], 0, 
 [Set to 1 if the system has Xrender.h.])
fi # if test "x$no_x" = "x"; then

AS_ECHO $LIB_XRENDER_ARG
AC_SUBST(HAVE_XRENDER_H)

# If we have $x_includes/X11/Xft/Xft.h, we should also make sure the
# compiler can find $x_includes/freetype2/ft2build.h, which is included
# in Xft.h.
if test "x$no_x" = "x"; then
  if test -f "$x_includes/X11/Xft/Xft.h"; then
    if test -f "$x_includes/freetype2/ft2build.h"; then
      lib_freetype_cflags="$lib_freetype_cflags -I $x_includes/freetype2"
    fi
  fi
fi

if test "x$have_glut_h_def" = "x-DHAVE_GLUT_H"; then
case $host_os in
     *darwin*)
     # Again, on OSX glut.h is contained in a framework.
     ;;
     *)
      x_ldflags="$x_ldflags -lGL -lGLU -lglut"
     ;;
esac
fi
   


if test "x$no_x" != "x"; then
   x_ldflags=""
fi

AC_DEFINE([CPP_AM_CFLAGS], [""], [Compiler flags for building the preprocessor.  Set by configure and used to create Makefiles.  You should not need to change this.])
AC_SUBST(CPP_AM_CFLAGS)
CPP_AM_CFLAGS="-g -Wall $use_gcc_includes $gcc_include_path"
CPP_AM_CFLAGS="$CPP_AM_CFLAGS $inline_function_opt $parser_opt_flags $have_off_t"
CPP_AM_CFLAGS="$CPP_AM_CFLAGS $gcc_stdint_wrapper $have_atoll_def $have_glob_def $have_glob_h_def $have_stdbool_def $have_localtime_r_def $no_fmt_trunc_warning"
CPP_AM_CFLAGS="$CPP_AM_CFLAGS $without_signed_hex_octal_constant_warnings_def"
CPP_AM_CFLAGS="$CPP_AM_CFLAGS $gnu_readline_cflags $no_builtins"
CPP_AM_CFLAGS="$CPP_AM_CFLAGS $use_old_builtins"

AC_DEFINE([LIB_AM_CFLAGS], [""], [Compiler flags for building libctalk.  Set by configure and used to create Makefiles.  You should not need to change this.])
AC_SUBST(LIB_AM_CFLAGS)
# LIB_AM_CFLAGS="$CPP_AM_CFLAGS $lib_opt_flags $lib_rt_flags $x_cflags $x_ldflags"

if test "$no_x" != yes; then
  LIB_AM_LDFLAGS="$LIB_AM_LDFLAGS $AM_LDFLAGS $x_ldflags"
fi  
LIB_AM_CFLAGS="$lib_opt_flags $lib_rt_flags $x_cflags"
LIB_AM_CFLAGS="$LIB_AM_CFLAGS -Wno-format-security"
LIB_AM_CFLAGS="$LIB_AM_CFLAGS $no_x11_def $x_protocol_errors"
LIB_AM_CFLAGS="$LIB_AM_CFLAGS $without_cleanup_errors_def"
LIB_AM_CFLAGS="$LIB_AM_CFLAGS $x11_warnings"
LIB_AM_CFLAGS="$LIB_AM_CFLAGS $have_localtime_r_def"
LIB_AM_CFLAGS="$LIB_AM_CFLAGS $gnu_readline_cflags"
LIB_AM_CFLAGS="$LIB_AM_CFLAGS $have_off_t $have_gnu_readline"
LIB_AM_CFLAGS="$LIB_AM_CFLAGS $have_atoll_def $have_glob_def $have_glob_h_def $have_stdbool_def $have_sys_socket_h_def $have_sys_un_h_def"
LIB_AM_CFLAGS="$LIB_AM_CFLAGS $without_circular_object_warnings"
LIB_AM_CFLAGS="$LIB_AM_CFLAGS $without_signed_hex_octal_constant_warnings_def"
LIB_AM_CFLAGS="$LIB_AM_CFLAGS $lib_freetype_cflags"
LIB_AM_CFLAGS="$LIB_AM_CFLAGS $have_glut_h_def $have_glx_h_def $have_glew_h_def "
LIB_AM_CFLAGS="$LIB_AM_CFLAGS $glwindowpos2i_def"
LIB_AM_CFLAGS="$LIB_AM_CFLAGS $freetype_includes"

AC_DEFINE([PARSER_AM_CFLAGS], [""], [Compiler flags for building the parser.  Set by configure and used to create Makefiles.  You should not need to change this.])
AC_SUBST(PARSER_AM_CFLAGS)
PARSER_AM_CFLAGS="$CPP_AM_CFLAGS $x_cflags $no_x11_def"

AC_PATH_PROG(cpp_path, cpp, no)
if test "$cpp_path" = "no" && test "$use_builtins" != "old" && test "$use_builtins" != "no"; then
AS_ECHO "NOTE:"
AS_ECHO "You must have the GNU preprocessor, cpp, installed in order to use"
AS_ECHO "GCC's built-in macro definitions, or use the --with-builtins=no option"
AS_ECHO "to build the preprocessor without any built-in macros."
AS_ECHO 
AS_ECHO "You can also use the option --with-builtins=old to try to use one of"
AS_ECHO "the old, system specific builtins definitions in ctpp/config."
exit 1
else

AC_PATH_PROG(su_path, su, no)
AC_SUBST(su_path)		      

if test -f "include/builtins.h"; then
   rm -f "include/builtins.h"
fi
if test "$use_builtins" != "no" && test "$use_builtins" != "old"; then
./build/builtins.sh
fi
fi

AC_OUTPUT(Makefile classes/Makefile lib/Makefile doc/Makefile src/Makefile
          classes/libc/Makefile classes/libctalk/Makefile ctpp/Makefile
	  libctpp/Makefile)

if test $make_generic_path != "no"; then

   AS_ECHO "/* This is a machine generated file.  Do not edit! */" >include/searchdirs.h
   AS_ECHO char *searchdirs[[]] = { >> include/searchdirs.h
   AS_ECHO  \"/usr/local/include\", >> include/searchdirs.h
   AS_ECHO  \"/usr/include\", >> include/searchdirs.h
   AS_ECHO '(void *)0' >> include/searchdirs.h
   AS_ECHO \}\; >> include/searchdirs.h
   AS_ECHO '#define __have_preloaded_searchdirs' >> include/searchdirs.h
   AS_ECHO '#define N_PATHS 2' >> include/searchdirs.h

   AS_ECHO > include_dirs.out
   AS_ECHO  /usr/local/include\ >> include_dirs.out
   AS_ECHO  /usr/include\ >> include_dirs.out

   AS_ECHO >> include_dirs.out

else

  if test $make_mit_posix_path == "yes"; then
  
    target_dir="$target_cpu-$target_os"

    if test $prefix == "NONE"; then
      prefix="/usr"
    fi

    this_libdir="$prefix/lib"

   AS_ECHO "/* This is a machine generated file.  Do not edit! */" >include/searchdirs.h
   AS_ECHO char *searchdirs[[]] = { >> include/searchdirs.h
   AS_ECHO  \"/usr/local/include\", >> include/searchdirs.h
   #
   # This is OUR version number, not gcc's because we probably can't 
   # locate the preprocessor....
   # If you don't like this, then use the --with-include-path option,
   # or else install gcc and then run ./configure without any options.

   #

   AS_ECHO \"$this_libdir/gcc/$target_dir/$VERSION/include\", >>include/searchdirs.h
   AS_ECHO \"/usr/$target_dir/include\", >>include/searchdirs.h
   AS_ECHO \"/usr/include\", >>include/searchdirs.h
   AS_ECHO '(void *)0' >> include/searchdirs.h
   AS_ECHO \}\; >> include/searchdirs.h
   AS_ECHO '#define __have_preloaded_searchdirs' >> include/searchdirs.h
   AS_ECHO '#define N_PATHS 4' >> include/searchdirs.h

   AS_ECHO  /usr/local/include > include_dirs.out
   # This is OUR version, not gcc's if we can't locate it....
   AS_ECHO $this_libdir/gcc/$target_dir/$VERSION/include >> include_dirs.out
   AS_ECHO /usr/$target_dir/include >>include_dirs.out
   AS_ECHO /usr/include >>include_dirs.out

  else

    if test $have_conf_include_path == "no"; then
      ./build/include_dirs.sh
    fi
  fi

fi

# This needs to be done after configure creates config.h.
AS_ECHO_N "adding system capabilities to classes/ctalklib... "
./build/caps.sh
AS_ECHO "done"

cat <<EOF
Configuring default compile and link options in ctcc\(1\) and ctdb\(1\).
EOF
./build/lcflags.sh "\"$def_ctalkflags\"" "\"$def_cflags\"" "\"$def_optflags\"" "\"$ctdb_ctcc_ldflags\"" "\"$ct_x_cflags\"" "\"$x_ldflags $lib_freetype_ld_opts $glut_ldflags $glew_ldflags\""

./build/confcttest.sh

if test -z $makeinfo_path; then
AC_MSG_WARN(Makeinfo Missing)
AC_MSG_WARN(Configure could not find the program \"makeinfo.\")
AC_MSG_WARN(Ctalk uses it to build its info documents.)
AC_MSG_WARN(You can find \"makeinfo,\" either as part of the)
AC_MSG_WARN(\"texinfo\" package in most UNIX/Linux distributions) 
AC_MSG_WARN(and also as a source code archive file from)
AC_MSG_WARN(http://www.gnu.org/.)
fi

AS_ECHO
AS_ECHO -----------------------------------------------------------
AS_ECHO
AS_ECHO "Static Libraries                      $enable_static"
AS_ECHO "Dynamic Libraries                     $enable_shared"
AS_ECHO_N "UNIX Domain Sockets                   "
if test "x$have_sys_socket_h_def" != "x"; then
   if test "x$have_sys_un_h_def" != "x"; then
     AS_ECHO yes 
   else
     AS_ECHO no
   fi
else
   AS_ECHO no
fi

AS_ECHO_N "GNU Readline Library                  "
if test "x$have_gnu_readline" != "x"; then
    AS_ECHO yes
else
    AS_ECHO no
fi    

AS_ECHO "X Render Extension                    $have_xrender"
AS_ECHO "Xft Outline Fonts                     $have_xft"
AS_ECHO "Freetype2 Font Library                $have_freetype2"
AS_ECHO "OpenGL 3D Graphics                    $gl_h_info"


AS_ECHO_N "GLUT OpenGL Utilities                 "
if test "x$GLUT_H_ARG" != "xno"; then
  AS_ECHO "yes"
else
  AS_ECHO "no"
fi

AS_ECHO_N "GLX OpenGL X Extension                "
if test "x$have_glx_h_def" = "x"; then
  AS_ECHO "no"
else
  AS_ECHO "yes"
fi

AS_ECHO_N "GLEW OpenGL Extension Library         "
if test "x$have_glew_h_def" = "x"; then
  AS_ECHO "no"
else
  AS_ECHO "yes"
fi

AS_ECHO

if test -f include_dirs.out; then
  AS_ECHO 'Built-In Include Paths:'
  cat include_dirs.out
  rm -f include_dirs.out
  AS_ECHO
else

   if test "x$CONF_INCLUDE_PATH"  != "x"; then
   AS_ECHO Built-In Include Path Option:
   echo
   AS_ECHO $CONF_INCLUDE_PATH
   echo
   fi

fi

   echo
   AS_ECHO "Default options for ctcc\(1\) and ctdb\(1\):"
   echo
   # AS_ECHO "CTALKFLAGS                "$def_ctalkflags
   cat <<EOF
CTALKFLAGS                $def_ctalkflags
EOF   

cat <<EOF
CFLAGS                    $def_cflags
                          $def_optflags
			  $no_restrict_warning
			  $no_stringop_overflow_warning
			  $stringop_truncation_warning
			  $address_of_packed_member_warning

EOF   
if test "x$no_x" != "xyes";  then
cat <<EOF
X CFLAGS                  $ct_x_cflags
EOF   
fi

if test "x$no_x" != "xyes";  then
cat <<EOF
LDFLAGS                   $ctdb_ctcc_ldflags
X LDFLAGS		  $x_ldflags $glut_ldflags $gl_ldflags
   			  $lib_freetype_ld_opts
EOF   
else
   AS_ECHO "LDFLAGS                   "$ctdb_ctcc_ldflags
fi
   echo
AS_ECHO -----------------------------------------------------------

./build/check_path.sh
