/* $Id: ctpp.h,v 1.2 2019/10/26 23:45:20 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011, 2018 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _CTPP_H
#define _CTPP_H

#ifndef _STDIO_H
#include <stdio.h>
#endif

#if 0
/*
  TODO - Removing this should wait until we have a bunch of OS's 
  to test it with, to make sure it doesn't break anything.
*/
#ifndef __BOOLEAN
#define __BOOLEAN
/*
 *  Compatible with X11/Intrinsic.h.
 */
#ifdef CRAY
typedef long Boolean;
#define False 0l
#define True !(False)
#else
typedef char Boolean;
#define False '\0'
#define True !(False)
#endif
#endif
#endif

#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
/* True and False are compatible with Xlib.h. */
#define True 1
#define False 0
#else
#ifndef __CT_BOOLEAN__
#define bool _Bool
#define true 1
#define false 0
#define True 1
#define False 0
#define TRUE true
#define FALSE false
#define __bool_true_false_are_defined	1
#define __CT_BOOLEAN__
#endif /* #ifndef __CT_BOOLEAN__ */
#endif /* #ifdef HAVE_STDBOOL_H */

#ifndef _STDARG_H
#include <stdarg.h>
#endif

#ifndef _PPARSER_H
#include "pparser.h"
#endif

#ifndef _PEXCEPT_H
#include "pexcept.h"
#endif

#ifndef _PLEX_H
#include "plex.h"
#endif

#ifndef _LIST_H
#include "list.h"
#endif

#ifndef _PMESSAGE_H
#include "pmessage.h"
#endif

#ifndef _PVAL_H
#include "pval.h"
#endif

#ifndef _PRTINFO_H
#include "prtinfo.h"
#endif

#ifndef _PSTRS_H_
#include "pstrs.h"
#endif

/*
 *  The number of buckets in a hash table should not need to be
 *  changed, unless the program needs to conserve memory.
 */
#ifndef _PHASH_H
# ifndef N_HASH_BUCKETS
# define N_HASH_BUCKETS 1024
# endif
# include "phash.h"
#endif

#ifndef MAXLABEL
#define MAXLABEL 255
#endif

#ifndef MAXMSG
#define MAXMSG 8192
#endif

#ifndef MAXUSERDIRS
#define MAXUSERDIRS 512
#endif

#define SUCCESS 0
#define ERROR -1

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE !(FALSE)
#endif
/*
 *  Error return if an exception requires a line or a conditional 
 *  clause to be skipped.
 */
#ifndef SKIP
#define SKIP 2
#endif

#ifndef MAXARGS
#define MAXARGS 512
#endif

#ifndef FILENAME_MAX
#define FILENAME_MAX 4096
#endif

/*
 *  Disk block size for reading and writing streams.  Linux uses
 *  1K blocks, and this should work nearly as well with other
 *  OS block sizes, although without optimization.
 */
#ifndef IO_BLKSIZE
#define IO_BLKSIZE 1024
#endif

#ifdef __DJGPP__
#define FILE_READ_MODE "r"
#define FILE_WRITE_MODE "w"
#else
#define FILE_READ_MODE "rb"
#define FILE_WRITE_MODE "wb"
#endif

#ifndef N_MESSAGES 
#define N_MESSAGES (MAXARGS * 120)
#endif

#ifndef P_MESSAGES
#define P_MESSAGES (N_MESSAGES * 30)  /* Enough to include all ISO headers. */
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef NULLSTR
#define NULLSTR "(null)"
#endif

#define VA_ARG_LABEL "__VA_ARGS__"
#define VA_ARG_PARAM "..."
#define N_ARG_LABEL "__NARGS__"

#define STR_VAL(s) _STR_VAL(s)
#define _STR_VAL(s) #s

#define MESSAGE_SIG  10101010
#define HASH_SIG     0xFF00
#define MACRODEF_SIG 0xFF01
#define HLIST_SIG    0xFF02

#define IS_MESSAGE(x) (((MESSAGE *)(x))->sig == MESSAGE_SIG)
#define IS_MACRODEF(x) (x -> sig == MACRODEF_SIG)
#define IS_HLIST(x) (x -> sig == HLIST_SIG)
#define M_NAME(m) (m -> name)
#define M_VAL(m)  (m -> value)
#define M_TOK(m) (m -> tokentype)

#define TRIM_LITERAL(s) (substrcpy (s, s, 1, strlen (s) - 2))
#define TRIM_CHAR(c)    (substrcpy (c, c, 1, strlen (c) - 2))

#define LINE_SPLICE(messages, i) (((i) < stack_start ((messages))) && \
                                  ((messages)[(i)+1]->name[0] == '\\'))

/*
 *  WHITESPACE tokens include the characters of isspace(3),
 *  except for newlines (\n and \r), which have their own token 
 *  type.
 */
#define M_ISSPACE(m) (((m) -> tokentype == WHITESPACE) || \
                      ((m) -> tokentype == NEWLINE))

/* Relies on the closure's order... might not help with threads. */
#define KEYWORD_CMP(s,t) ((*s == *t) && !strcmp (s, t))

#ifdef __GNUC__
#define KEYWORD_INCLUDE 1
#define KEYWORD_INCLUDE_NEXT 2
#define KEYWORD_IDENT 3
#define KEYWORD_SCCS 4
#define KEYWORD_UNASSERT 5
#define KEYWORD_DEFINE 6
#define KEYWORD_UNDEF 7
#define KEYWORD_IF 8
#define KEYWORD_IFDEF 9
#define KEYWORD_IFNDEF 10
#define KEYWORD_ELSE 11
#define KEYWORD_ELIF 12
#define KEYWORD_ENDIF 13
#define KEYWORD_ERROR 14
#define KEYWORD_WARNING 15
#define KEYWORD_ASSERT 16
#define KEYWORD_LINE 17
#define KEYWORD_PRAGMA 18
#define KEYWORD_INCLUDE_HERE 19
#else
#define KEYWORD_INCLUDE 1
#define KEYWORD_DEFINE 2
#define KEYWORD_UNDEF 3
#define KEYWORD_IF 4
#define KEYWORD_IFDEF 5
#define KEYWORD_IFNDEF 6
#define KEYWORD_ELSE 7
#define KEYWORD_ELIF 8
#define KEYWORD_ENDIF 9
#define KEYWORD_ERROR 10
#define KEYWORD_WARNING 11
#define KEYWORD_ASSERT 12
#define KEYWORD_LINE 13
#define KEYWORD_PRAGMA 14
#endif

#ifdef __DJGPP__
# ifndef S_SPLINT_S
# define SNPRINTF(_buf,_size,_fmt,...) sprintf (_buf,_fmt,__VA_ARGS__)
# else
# define SNPRINTF(_buf,_size,_fmt) 
# endif
#else
# ifndef S_SPLINT_S
# define SNPRINTF(_buf,_size,_fmt,...) snprintf (_buf,_size,_fmt,__VA_ARGS__)
# else 
# define SNPRINTF(_buf,_size,_fmt) sprintf (_buf, _fmt)
#endif
#endif

/*
 * Internationalization
 */
#ifndef _
#define _(String) (String)
#endif

#ifndef N_
#define N_(String) (String)
#endif

#ifndef textdomain
#define textdomain(Domain)
#endif

#ifndef bindtextdomain
#define bindtextdomain(Package,Directory)
#endif

/*
 *  Rules for outputting make rule.
 */
#define MAKERULE             (1 << 0)
#define MAKERULELOCALHEADER  (1 << 1)
#define MAKERULEGENHEADER    (1 << 2)
#define MAKERULETOFILE       (1 << 3)
#define MAKERULEUSERTARGET   (1 << 4)

#define MAKE_TARGET_EXT      ".o"
#define MAKE_TARGET_EXT_LEN 2

#define TMPFILE_PFX     "c."             /* Generic tmp prefix.            */

#if defined (__sparc__) && defined (__GNUC__)
#define index strchr
#define rindex strrchr
#endif

/* Does not include entries for -traditional or -traditional-cpp. */
typedef enum {
  nolinemarker_def = 0,
  nostdinc_def = 1,
  keep_pragma_def = 2,
  warndollar_def = 3,
  warnundefsymbols_def = 4,
  keepcomments_def = 5,
  warnnestedcomments_def = 6,
  unassert_def = 7,
  definestooutput_def = 8,
  definestofile_def = 9,
  definenamesonly_def = 10,
  definesonly_def = 11,
  makerule_def = 12,
  undef_builtin_def = 13,
  use_trigraphs_def = 14,
  warn_trigraphs_def = 15,
  warn_all_def = 16,
  lang_cplusplus_def = 17,
  no_warnings_def = 18,
  verbose_def = 19,
  warnings_to_errors_def = 20,
  print_headers_def = 21,
  pre_preprocessed_def = 22,
  gcc_macros_def = 23,
  warnunused_def = 24,
  no_include_def = 25,
  no_simple_macros_def = 26,
  move_includes_def = 27,
  warn_missing_args_def = 28
} BUILTIN_OPT;

/*
 *  Legal characters in an include path, including path characters, 
 *  but not quote and angle bracket delimiters.  Some of the characters 
 *  are GNU idioms.
 */
#define INC_PATH_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_/.-+"

/* 
 *    Debugging 
 *    Define DEBUG_CODE and any of symbols below.
 */
#undef DEBUG_CODE

/* 
 *    #define STACK_TRACE if you want the interpreter to print 
 *    huge amounts of information about the stack values during 
 *    processing.  MACRO_STACK_TRACE reports what is on the 
 *    preprocessor stack.
 */
#undef STACK_TRACE
/*
 *    TRACE_PREPROCESS_INCLUDES prints the include level
 *    of each #included file.
 */
#undef TRACE_PREPROCESS_INCLUDES
/*
 *    CHECK_PREPROCESS_CONDITIONALS prints the level of
 *    nested conditionals before and after every #include
 */
#undef CHECK_PREPROCESS_IF_LEVEL
/* 
 *    DEBUG_SYMBOLS prints information about the symbols
 *    defined by the preprocessor.  If a symbol is not 
 *    defined, print a warning - GNU cpp silently returns
 *    false for undefined symbols.
 */
#undef DEBUG_SYMBOLS
/*
 *    Issue a warning for undefined preprocessor symbols.
 *    GNU cpp returns false without issuing a warning when
 *    evaluating an undefined symbol.
 */
#undef DEBUG_UNDEFINED_PREPROCESSOR_SYMBOLS
/*
 *    DEBUG_TYPEDEFS dumps a list of the typedefs.
 */
#undef DEBUG_TYPEDEFS
/*    DEBUG_C_VARIABLES causes the interpreter to perform more
 *    type checking.
 */
#undef DEBUG_C_VARIABLES
/*
 *    FN_STACK_TRACE prints a stack trace of the function output
 *    buffer.
 */
#undef FN_STACK_TRACE
/*
 *    MALLOC_TRACE enables the glibc mtrace () function.  You
 *    must also set the environment variable MALLOC_TRACE to
 *    the name of the file that will contain the trace data.
 */
#undef MALLOC_TRACE

/* Prototypes */

/* assert.c */
int check_assertion (MESSAGE_STACK, int, int *, VAL *);
int define_assertion (MESSAGE_STACK, int);
int is_assertion (char *);
int str_is_expr (char *);
int eval_assertion (char *, char *);

/* a_opt.c */
int assert_opt (char **, int, int);
void handle_unassert (void);

/* ansisymbols.c */
void ansi_symbol_init (void);
void ansi__func__ (char *);
void ansi__FILE__ (char *);
void ansi__LINE__ (int);
int get_ansi__LINE__ (void);

/* builtins.c */
int gnu_builtins (void);
int is_builtin_symbol (char *);
void opt_builtins (void);

/* ccompat.c */
void gcc_lib_subdir_warning (void);
void ccompat_init (void);
void find_gpp_subdir (void);
int gpp_version_subdir (char *);
int gcc_lib_path_from_compiler (void);
void gnu_attributes (MESSAGE_STACK, int);
void gcc_builtins (void);
int is_gnuc_symbol (char *);
int is_solaris10 (void);
void fixup_wrapped_gcc_stdints_linux (void);
int sol10_attribute_macro_sub (DEFINITION *);
void sol10_compat_defines (void);
int sol10_use_gnuc_stdarg_h (INCLUDE *);
int wrapped_gcc_stdints_dup_osx (const char *);
#if defined CONF_INCLUDE_PATH
void cleanup_conf_include_searchdirs (void);
#endif

/* cexpr.c */
void parse_include_directive (MESSAGE_STACK, int, int, INCLUDE *);
int eval_constant_expr (MESSAGE_STACK, int, int *, VAL *);
int expr_reparse (MESSAGE_STACK, int, int *, VAL *);

/* chkstate.c */
int check_state (int, MESSAGE **, int *, int);

/* d_opt.c */
int define_opt (char **, int, int);
int set_defines_file_name (char **, int, int);

/* error.c */     
#ifdef DEBUG_CODE
void debug (char *fmt,...);
#endif
char *source_filename (void);
void error (MESSAGE *, char *,...);
void warning (MESSAGE *, char *,...);
void parser_error (MESSAGE *, char *,...);
void location_trace (MESSAGE *);

/* escline.c */
int escaped_line_end (MESSAGE **, int, int);

/* i_opt.c */
void define_imacros (void);
void process_include_opt_files (void);
int imacro_filename (char **, int, int);
int include_filename (char **, int, int);
int include_dirafter_name (char **, int, int);
int include_dirafter_name_prefix (char **, int, int);
int include_prefixafter_name (char **, int, int);
int include_systemdir_name (char **, int, int);

/* include.c */
void dump_include_paths (void);
int env_paths (void);
void init_include_paths (void);
char *find_include (char *, int);
int split_path_var (char *);
bool has_include (char *, bool);

/* is_fn.c */
int is_c_function_prototype_declaration (MESSAGE_STACK, int);
int is_c_func_declaration_msg (MESSAGE_STACK, int);

/* is_methd.c */
int is_instance_method_declaration_start (MESSAGE_STACK, int, int);

/* lineinfo.c */
int expand_line_args (MESSAGE_STACK, int);
char *fmt_line_info (int, char *, int, int, char *);
int line_directive (MESSAGE_STACK, int);

/* m_opt.c */
int add_user_target (char *);
void include_dependency (char *);
int make_target (void);
int makerule_filename (char **, int, int);
void check_rules_file (void);
int output_make_rule (void);

/* macprint.c */
void output_symbols (void);
void output_symbol_names (void);
void output_unused_symbols (void);
int write_defines (void);

/* main.c */
void parse_args (int, char **);
void help (void);
int include_opt (char **, int, int);
int extended_arg (char *);
void verbose_info (void);
void input_lang_from_file (char *);
int std_opt (char **, int, int);

/* pcvars.c */
int pmatch_type (VAL *, VAL *);

/* pmath.c */
bool eval_bool (MESSAGE_STACK, int, VAL *);
int eval_math (MESSAGE_STACK, int, VAL *);
int perform_add (MESSAGE *, VAL *, VAL *, VAL *);
int perform_subtract (MESSAGE *, VAL *, VAL *, VAL *);
int perform_multiply (MESSAGE *, VAL *, VAL *, VAL *);
int perform_divide (MESSAGE *, VAL *, VAL *, VAL *);
int perform_asl (MESSAGE *, VAL *, VAL *, VAL *);
int perform_asr (MESSAGE *, VAL *, VAL *, VAL *);
int perform_bit_and (MESSAGE *, VAL *, VAL *, VAL *);
int perform_bit_or (MESSAGE *, VAL *, VAL *, VAL *);
int perform_bit_xor (MESSAGE *, VAL *, VAL *, VAL *);
int perform_bit_comp (MESSAGE *, VAL *, VAL *);
bool question_conditional_eval (MESSAGE_STACK, int, int *, VAL *);
int handle_sizeof_op (MESSAGE_STACK, int, int *, VAL *);

/* ppexcept.c */
int handle_preprocess_exception (MESSAGE_STACK, int, int, int);
void new_exception (EXCEPTION);
void set_preprocess_env (void);

/* ppop.c */
OP_CONTEXT op_context (MESSAGE_STACK, int);
int op_precedence (int, MESSAGE_STACK, int);
int operands (MESSAGE_STACK, int, int *, int *);

/* pragma.c */
int handle_pragma_directive (MESSAGE_STACK, int);
int handle_stdc_pragma (MESSAGE_STACK, int);
int handle_gcc_pragma (MESSAGE_STACK, int);
int handle_pragma_op (MESSAGE_STACK, int);
void init_poison_identifiers (void);

/* preprocess.c */
int stack_start (MESSAGE_STACK);
int get_stack_top (MESSAGE_STACK);
int p_message_push (MESSAGE *);
MESSAGE *p_message_pop (void);
int t_message_push (MESSAGE *);
MESSAGE *t_message_pop (void);
int a_message_push (MESSAGE *);
MESSAGE *a_message_pop (void);
int i_message_push (MESSAGE *);
MESSAGE *i_message_pop (void);
int v_message_push (MESSAGE *);
MESSAGE *v_message_pop (void);
DEFINITION *add_symbol (char *, char *, MACRO_ARG **);
void adj_include_stack (int);
int include_return (int);
int line_info (int, char *, int, int);
void include_to_stack (MESSAGE_STACK, INCLUDE *, char *, int, int);
int include_file (MESSAGE_STACK, int, int);
int include_next_file (MESSAGE_STACK, int, int);
int macro_sub_parse (MESSAGE_STACK, int, int *, VAL *);
int eval_conditional (MESSAGE *, int);
int end_of_p_stmt (MESSAGE_STACK, int);
int macro_parse (MESSAGE_STACK, int, int *, VAL *);
int move_includes (MESSAGE_STACK, int, int *);
void init_preprocess_if_vals (void);
int handle_ifdef (MESSAGE_STACK, int, VAL *);
int handle_ifndef (MESSAGE_STACK, int, VAL *);
int handle_defined_op (MESSAGE_STACK, int, int *, VAL *);
void p_hash_loc (MESSAGE *);
void preprocess_error (MESSAGE_STACK, int);
int preprocess_warning (MESSAGE_STACK, int);
int preprocess_message (MESSAGE_STACK, int);
int undefine_symbol (MESSAGE_STACK, int);
int define_symbol (MESSAGE_STACK, int);
DEFINITION *sub_symbol (DEFINITION *);
#if (__GNUC__ < 3)
void ptr_of_stack (const MESSAGE_STACK, long int **);
#else
void ptr_of_stack (const MESSAGE_STACK, long int **) __attribute__ ((always_inline));
#endif
int resolve_symbol (char *, VAL *);
DEFINITION *get_symbol (char *, int);
int rescan_arg (DEFINITION *, MESSAGE_STACK, int, int *);
int replace_args (DEFINITION *, MESSAGE_STACK, int, int *);
DEFINITION *replace_macro_arg_scan1 (DEFINITION *, MESSAGE_STACK, int);
int replace_macro (DEFINITION *, MESSAGE_STACK, int, int *);
void splice_stack (MESSAGE_STACK, int, int);
void splice_stack2 (MESSAGE_STACK, int, int, int *);
void insert_stack (MESSAGE_STACK, int, MESSAGE_STACK, int, int);
void insert_stack2 (MESSAGE_STACK, int, MESSAGE_STACK, int, int, int *);
int literalize_token (MESSAGE_STACK, int *);
int literalize_arg (MESSAGE_STACK, DEFINITION *, int, int, int, int, char **);
int concatenate_tokens (MESSAGE_STACK, int *, CONCAT_MODE);
int concatenate_args (MESSAGE_STACK, DEFINITION *, int, int, int, char **);
void tokenize_define (char *);
int next_label (MESSAGE_STACK, int);
void dump_symbols (void);
int check_preprocess_state (int, MESSAGE **, int *, int);
void preprocess (char *);
int skip_conditional (MESSAGE_STACK, int);
int exception_adj (MESSAGE_STACK, int);
int replace_va_args (MESSAGE_STACK, DEFINITION *, int, int, int, char **);
int macro_is_variadic (DEFINITION *);
MACRO_ARG *new_arg (void);
int replacement_concat_modes (MESSAGE_STACK, int, MACRO_ARG **);

/* psubexpr.c */
int p_match_paren (MESSAGE **, int, int);
int match_paren (MESSAGE **, int, int);
int nextlangmsg (MESSAGE **, int);
int prevlangmsg (MESSAGE **, int);
int scanforward_other (MESSAGE **, int, int, int);
int scanforward (MESSAGE **, int, int, int);
int _scanforward (MESSAGE **, int, int, int, int);
int scanback (MESSAGE **, int, int, int);
int scanback_other (MESSAGE **, int, int, int);
int _scanback (MESSAGE **, int, int, int, int);
int macro_subexpr (MESSAGE **, int, int *, VAL *);
int set_subexpr_val (MESSAGE_STACK, int, VAL *);
int constant_subexpr (MESSAGE_STACK, int, int *, VAL *);

/* type_of.c */
int p_type_of (char *);

/* u_opt.c */
void perform_undef_macro_opt (void);
void undefine_builtin_macros (void);
int undefine_macro_opt (char **, int, int);

/* libctpp/ansitime.c */
char *mon (int);

/* libctpp/bintodec.c */
int ascii_bin_to_dec (char *);

/* libctpp/bnamecmp.c */
int basename_cmp (char *, char *);
char *mbasename (char *);

/* libctpp/error_out.c */
void _error_out (char *);

/* libctpp/errorloc.c */
void error_reset (void);

/* libctpp/hash.c */
void _new_hash (HASHTAB *);
HLIST *_new_hlist (void);
void *_hash_get (HASHTAB, char *);
void _hash_put (HASHTAB, void *, char *);
void *_hash_remove (HASHTAB, char *);
void macrodefs_hash_init (void);
void _hash_symbol_line (int);
void _hash_symbol_column (int);
void _hash_symbol_source_file (char *);
HLIST *_hash_first (HASHTAB *);
HLIST *_hash_next (HASHTAB *);

/* libctpp/keyword.c */
int is_ctalk_keyword (const char *);
int is_macro_keyword (const char *);
int is_c_keyword (const char *);
int is_c_data_type (const char *);
int is_c_storage_class (const char *);
int is_extension_keyword (const char *);
int is_ctrl_keyword (const char *);

/* libctpp/lex.c */
int esc_quote_mismatch_idx (char *, int);
int lexical (char *, long long *, MESSAGE *);
int tokenize (int (*)(MESSAGE *), char *);
int tokenize_reuse (int (*)(MESSAGE *), char *);
int prefix (MESSAGE_STACK, int, int);
int set_error_location (MESSAGE *, int *, int *);
int quote_mismatch_idx (char *, int);
char *collect_tokens (MESSAGE_STACK, int, int);
char unescape_trigraph (char *);
int trigraph_tokentype (char);
int is_char_constant (char *);
int unterm_str_warning (int);

/* libctpp/lextype.c */
int _lextype (const char *);

/* libctpp/lineinfo.c */
int l_message_push (MESSAGE *);
MESSAGE *l_message_pop (void);
int line_info_tok (char *);
int set_line_info (MESSAGE_STACK, int, int);

/* libctpp/list.c */
void list_add (LIST *, LIST *);
LIST *new_list (void);
void list_push (LIST **, LIST **);
LIST *list_unshift (LIST **);
void delete_list_element (LIST *);
void delete_list (LIST **);

/* libctpp/message.c */
MESSAGE *new_message (void);
void delete_message (MESSAGE *);
void copy_message (MESSAGE *, MESSAGE *);
MESSAGE *dup_message (const MESSAGE *);
MESSAGE *resize_message (MESSAGE *, int);
void reuse_message (MESSAGE *m);
MESSAGE *get_reused_message (void);
void cleanup_reused_messages (void);

/* libctpp/radixof.c */
RADIX radix_of (const char *);

/* libctpp/read.c */
#if defined(__CYGWIN32__) || defined(__DJGPP__) || defined(__APPLE__)
int read_file (char **, char *);
#else
size_t read_file (char **, char *);
#endif

/* libctpp/rt_error.c */
void _error (char *, ...);
void _warning (char *, ...);

/* libctpp/rtinfo.c */
char *__argvFileName (void);
void __source_file (char *);
char *__source_filename (void);
void __init_time (void);
void set_traditional (void);
int symbol_is_command_line_builtin (char *);

/* libctpp/rtsyserr.c */
int __ctalkErrnoInternal (void);

/* libctpp/sformat.c */
char *_format_str (char *, char *, va_list);

/* libctpp/statfile.c */
int file_exists (char *);
int file_size (char *);
int is_dir (char *path);
char *which (char *);

/* libctpp/strcatx.c */
/* these causes a warning with some versions of GCC, so
   only use where strcatx is called */
#if 0
#ifdef __GNUC__
inline int strcatx (char *destbuf, ...);
#else
int strcatx (char *destbuf, ...);
#endif  
#ifdef __GNUC__
inline int strcatx2 (char *destbuf, ...);
#else
int strcatx2 (char *destbuf, ...);
#endif  
#endif  /* #if 0 */

/* libctpp/substrcpy.c */
char *substrcpy (char *, char *, int, int);
char *substrcat (char *, char *, int, int);

/* libctpp/tempio.c */
void init_tmp_files (void);
void create_tmp (void);
int unlink_tmp (void);
int close_tmp (void);
int write_tmp (char *);
char *get_tmpname (void);
void tmp_to_output (void);
int rename_file (char *, char *);
void copy_file (char *oldname, char *newname);
void cleanup (int);

/* libctpp/trimstr.c */
void trim_leading_whitespace (char *);
void trim_trailing_whitespace (char *);

/* libctpp/val.c */
void m_print_val (MESSAGE *, VAL *);
int d_print_val (DEFINITION *, VAL *);
int copy_val (VAL *, VAL *);
int is_val_true (VAL *);
int numeric_value (const char *, VAL *);
int val_eq (VAL *, VAL *);

/* xalloc.c */
void *__xalloc (int size);
void __xfree(void *__p);

#endif /* _CTPP_H */
