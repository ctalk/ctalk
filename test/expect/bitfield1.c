/* this might take some adjusting depending on which OS X version this is
   tested on - __darwin_fp_control may or may not be part of
   /usr/include/mach/i386/_structs.h. */
/* If this generates an redefinition error on your mac, then just 
   comment the definition out */

#ifndef __APPLE__
struct __darwin_fp_control
{
    unsigned short		__invalid	:1,
    				__denorm	:1,
				__zdiv		:1,
				__ovrfl		:1,
				__undfl		:1,
				__precis	:1,
						:2,
				__pc		:2,
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define FP_PREC_24B		0
#define	FP_PREC_53B		2
#define FP_PREC_64B		3
#endif /* !_POSIX_C_SOURCE || _DARWIN_C_SOURCE */
				__rc		:2,
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define FP_RND_NEAR		0
#define FP_RND_DOWN		1
#define FP_RND_UP		2
#define FP_CHOP			3
#endif /* !_POSIX_C_SOURCE || _DARWIN_C_SOURCE */
					/*inf*/	:1,
						:3;
};
#endif

int main () {
  struct __darwin_fp_control dfc;
  CVAR *c, *t;
  int i_int;
  Integer new i;

  i_int = dfc.__rc;
  i = i_int;

  printf ("ok\n");
}
