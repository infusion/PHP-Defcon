/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Robert Eisele <robert@xarg.org>                              |
  | Site: http://www.xarg.org/project/php-defcon/                        |
  +----------------------------------------------------------------------+
*/

// set to 1 to enable copious "stderr" debug output during parse
#define DEBUG_OUTPUT 0

#include <sys/types.h>
#include <dirent.h>
#include <stddef.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_defcon.h"

static zend_function_entry defcon_functions[] = {
	{NULL, NULL, NULL}
};

PHP_INI_BEGIN()
PHP_INI_ENTRY("defcon.config-file", "/etc/defcon.conf", PHP_INI_SYSTEM, NULL )
PHP_INI_END()

zend_module_entry defcon_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_DEFCON_EXTNAME,
    defcon_functions,
    PHP_MINIT(defcon),
    PHP_MSHUTDOWN(defcon),
    NULL,
    NULL,
    PHP_MINFO(defcon),
#if ZEND_MODULE_API_NO >= 20010901
    PHP_DEFCON_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_DEFCON
ZEND_GET_MODULE(defcon)
#endif

PHP_MINFO_FUNCTION(defcon) {
    php_info_print_table_start();
    php_info_print_table_row(2, "defcon support", "enabled");
    php_info_print_table_row(2, "defcon version", PHP_DEFCON_VERSION);
    php_info_print_table_end();
}

enum defcon_state_id {
	ST_KEYWORD	= 0,
	ST_CONST_NAME	= 1,
	ST_CONST_EQUAL	= 2,
	ST_CONST_VALUE	= 3,
	ST_CONST_TERM	= 4,
	ST_REQUIRE_PATH	= 5,
	ST_REQUIRE_TERM	= 6,
};

enum defcon_keyword_id {
	KW_INVALID	= -1,
	KW_STRING	= 0,
	KW_INT		= 1,
	KW_LONG		= 2,
	KW_FLOAT	= 3,
	KW_REAL		= 4,
	KW_DOUBLE	= 5,
	KW_BOOL		= 6,
	KW_BOOLEAN	= 7,
	KW_LOGICAL	= 8,
	KW_SHORT	= 9,
	KW_REQUIRE	= 10,
	KW_INCLUDE	= 11,
};

struct defcon_keyword {
	char *name;
	int state;	// state to switch to when found
	int may_concat;	// may the value be build using '.' concatenation?
};

static struct defcon_keyword keywords[] = {
[KW_STRING] =	{ "string",	ST_CONST_NAME,		1 },
[KW_INT] =	{ "int",	ST_CONST_NAME,		0 },
[KW_LONG] =	{ "long",	ST_CONST_NAME,		0 },
[KW_FLOAT] =	{ "float",	ST_CONST_NAME,		0 },
[KW_REAL] =	{ "real",	ST_CONST_NAME,		0 },
[KW_DOUBLE] =	{ "double",	ST_CONST_NAME,		0 },
[KW_BOOL] =	{ "bool",	ST_CONST_NAME,		0 },
[KW_BOOLEAN] =	{ "boolean",	ST_CONST_NAME,		0 },
[KW_LOGICAL] =	{ "logical",	ST_CONST_NAME,		0 },
[KW_SHORT] =	{ "short",	ST_CONST_NAME,		0 },
[KW_REQUIRE] =	{ "require",	ST_REQUIRE_PATH,	1 },
[KW_INCLUDE] =	{ "include",	ST_REQUIRE_PATH,	1 },
};
#define NR_KW (sizeof(keywords)/sizeof(keywords[0]))

static enum defcon_keyword_id match_keyword(
	const char *kw
) {
	enum defcon_keyword_id id;

	for (id = 0; id < NR_KW; id++) {
		if (0 == strcasecmp(keywords[id].name, kw))
		return id;
	}
	return KW_INVALID;
}


struct defcon_context {
	int module_number;
	char *file;
	int line;
};

#define PR_ERR(CTX, FMT, ...) \
	php_error(E_ERROR, "defcon: %s line %i: " FMT, \
		 (CTX)->file, (CTX)->line, ## __VA_ARGS__)
#define PR_WARN(CTX, FMT, ...) \
	php_error(E_WARNING, "defcon: %s line %i: " FMT, \
		 (CTX)->file, (CTX)->line, ## __VA_ARGS__)
#define PR_NOTICE(CTX, FMT, ...) \
	php_error(E_NOTICE, "defcon: %s line %i: " FMT, \
		 (CTX)->file, (CTX)->line, ## __VA_ARGS__)
#if DEBUG_OUTPUT
#define PR_DBG(CTX, FMT, ...) \
	fprintf(stderr, "DEFCON %s:%i " FMT, \
		(CTX)->file, (CTX)->line, ## __VA_ARGS__)
#else
#define PR_DBG(CTX, FMT, ...) do {} while (0)
#endif

static inline zval *find_constant(
	char *N,
	int Nlen
) {
	zval *Z[1];

	return zend_hash_find(EG(zend_constants), N, Nlen+1, (void **)Z)
		== SUCCESS ? *Z : 0;
}

static void add_constant(
	struct defcon_context *ctx,
	enum defcon_keyword_id KW,
	char *N,
	char *V,
	int Vlen
TSRMLS_DC) {
	zend_constant zc;

	switch (KW) {
	   case KW_STRING:
		zc.value.type = IS_STRING;
		zc.value.value.str.val = zend_strndup(V, Vlen);
		zc.value.value.str.len = Vlen;
		break;
	   case KW_INT:
	   case KW_LONG:
	   case KW_SHORT:
		zc.value.type = IS_LONG;
		zc.value.value.lval = atol(V);
		break;
	   case KW_FLOAT:
	   case KW_REAL:
	   case KW_DOUBLE:
		zc.value.type = IS_DOUBLE;
		zc.value.value.dval = atof(V);
		break;
	   case KW_BOOL:
	   case KW_BOOLEAN:
	   case KW_LOGICAL:
		zc.value.type = IS_BOOL;
		if (0 == strcasecmp(V, "true")) {
			zc.value.value.lval = 1;
		} else if (0 == strcasecmp(V, "false")) {
			zc.value.value.lval = 0;
		} else {
			zc.value.value.lval = atol(V);
		}
		break;
	}
	zc.flags = CONST_CS | CONST_PERSISTENT;
	zc.name = zend_strndup(N, strlen(N));
	zc.name_len = strlen(N)+1;
	zc.module_number = ctx->module_number;

	if (zend_register_constant(&zc TSRMLS_CC) == FAILURE)
		PR_ERR(ctx, "FAILED: define('%s', '%.*s')\n", N, Vlen, V);
	else
		PR_DBG(ctx, "DONE: define('%s', '%.*s')\n", N, Vlen, V);
}

// given a string in V[Vlen+Nlen], use the substring starting at V[Vlen]
// to look up an already defined constant, and if it is already defined,
// replace the string in V[Vlen...Vlen+Nlen] with that constant's value.
// If the resulting total string would become too long, truncate the
// replacement value.
//
// Returns the new overall length of the string in V[], whether replacement
// was done, or not.
static int replace_constant(
	struct defcon_context *ctx,
	char *V,
	int Vlen,
	int Nlen
) {
	zval *Z;
	char *newV, strV[VALUELEN+1];
	int newVlen;

	if (0 == (Z = find_constant(V+Vlen, Nlen)))
no_replace:	return Vlen+Nlen;

	switch (Z_TYPE_P(Z)) {
	   case IS_NULL:
		return Vlen;	// NULL: replacement is empty string
	   case IS_LONG:
		sprintf(strV, "%ld", Z_LVAL_P(Z));
		break;
	   case IS_DOUBLE:
		// NOTE: the %.13f format _seems_ to be what PHP does
		// when converting double to string. I cannot find any
		// documentation on that, though.
		sprintf(strV, "%.13f", Z_DVAL_P(Z));
		break;
	   case IS_BOOL:
		if (!Z_BVAL_P(Z))
			return Vlen;	// false: replacement is empty string
		// true: replacement is string "1"
		newV = "1";
		newVlen = 1;
		goto use_newV;
	   case IS_STRING:
		newV = Z_STRVAL_P(Z);
		newVlen = Z_STRLEN_P(Z);
		goto use_newV;
	   default:
		goto no_replace;
	}
	newV = strV;
	newVlen = strlen(newV);
use_newV:
	if (Vlen + newVlen > VALUELEN)
		newVlen = VALUELEN - Vlen;
	memcpy(V+Vlen, newV, newVlen);
	Vlen += newVlen;
	V[Vlen] = '\0';
	return Vlen;
}

// given a \0-terminated string in V[Vlen+Nlen], use the substring starting
// at V[Vlen] as a shell command, read the output of that shell command,
// and then replace the string in V[Vlen...Vlen+Nlen] with that output text.
// If the output of the command would make the resulting total string in
// V[] longer than VALUELEN, it will be truncated.
//
// The backtick replacement works ALMOST as in PHP, but for your convenience,
// a terminating newline will be removed from the replacement string, as it
// will almost never be wanted. If you need it, make your command output
// an additional newline.
//
// Returns the new overall length of the whole string in V[].
static int replace_shellcommand(
	struct defcon_context *ctx,
	char *V,
	int Vlen,
	int Nlen
) {
	FILE *fp = popen(V+Vlen, "r");
	char newV[VALUELEN+1];
	int have, got;

	if (!fp)
		return Vlen+Nlen;
	for (have = 0; !feof(fp) && have < VALUELEN; have += got)
		got = fread(newV + have, 1, VALUELEN - have, fp);
	pclose(fp);
	if (have > 0 && newV[have-1] == '\n') have--;
	if (have > VALUELEN - Vlen) have = VALUELEN - Vlen;
	memcpy(V + Vlen, newV, have);
	Vlen += have;
	V[Vlen] = '\0';
	return Vlen;
}

#define WS(C) (C == ' ' || C == '\n' || C == '\t' || C == '\r')
#define SEP(C) (C == ',' || C == ';')
#define QUOTE(C) (C == '\'' || C == '"' || C == '`')
#define ALPHA(C) ((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || C == '_')
#define ALNUM(C) (ALPHA(C) || (C >= '0' && C <= '9'))

// Match the input at *sp as a keyword.
// The keyword matched, is returned in kw[KEYWORDLEN+1], \0-terminated.
// The return value is a keyword id, possibly KW_INVALID.
static enum defcon_keyword_id parse_keyword(
	struct defcon_context *ctx,
	char **sp,
	char *kw
) {
	int i;
	enum defcon_keyword_id kwid;

	for (i = 0; ALPHA(**sp); (*sp)++, i++) {
		if (i >= KEYWORDLEN) {
			PR_ERR(ctx, "Keyword too long");
			return KW_INVALID;
		}
		kw[i] = **sp;
	}
	kw[i] = '\0';
	kwid = match_keyword(kw);
	if (KW_INVALID == kwid)
		PR_ERR(ctx, "No valid keyword (%s)", kw);
	return kwid;
}

// Match the input at *sp as a constant name. Check whether that constant
// is not already defined.
//
// If all is well, 1 is returned, and the name is stored at N, \0-terminated.
//
// If the constant already exists and should not be redefined, 0 is returned.
// A suitable error message is logged, unless the given constant name
// started with a '@' shutup operator.
//
// If an invalid constant name is detected, -1 is returned, after a suitable
// error message is logged.
//
//
static int parse_constantname(
	struct defcon_context *ctx,
	char **sp,
	char *N
) {
	int i, shutup = 0;

	if (**sp == '@') {
		(*sp)++;
		shutup = 1;
	}
	if (!ALPHA(**sp)) {
		PR_ERR(ctx, "No Constant name set");
		return -1;
	}
	for (i = 0; ALNUM(**sp); (*sp)++, i++) {
		if (i >= NAMELEN) {
			PR_ERR(ctx, "Constant name too long");
			return -1;
		}
		N[i] = **sp;
	}
	N[i] = '\0';
	if (KW_INVALID != match_keyword(N)) {
		PR_ERR(ctx, "Constant name '%s' should not be a keyword", N);
		return -1;
	}
	if (find_constant(N, i)) {
		if (!shutup)
			PR_NOTICE(ctx, "Constant '%s' redefined", N);
		return 0;
	}

	return 1;
}

// given a single character in c, either return -1 if it is not a
// valid octal digit (0-7), or return its numeric value.
static int oct_digit(int c)
{
	if (c >= '0' && c <= '7')
		return c - '0';
	return -1;
}

// given a single character in c, either return -1 if it is not a
// valid hexadecimal digit (0-7a-fA-F), or return its numeric value.
static int hex_digit(int c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a';
	if (c >= 'A' && c <= 'F')
		return c - 'A';
	return -1;
}

// Parse a quoted string value. The leading quote character has already
// been skipped over. Its character value, is given in the 'quote' argument.
//
// There are three different ways to quote:
// - single quotes, with almost no backslash escaping
// - double quotes, with full backslash escaping
// - backtick quotes, like double quotes, running the thing quoted
//   as a shell command and substituting the output of that run.
//
// Quoted strings continue until the matching closing quote character
// is found, even over newlines, which result in the newline being
// included in the string content.
//
// Within the quoted string, a backslash is interpreted specially,
// following the usual PHP conventions: a '\' followed by the
// active quote character, results in the quote character being
// put into the target string. A double backslash must be used to
// include the backslash itself into the target string. Only for
// double quoted strings, the usual additional escape sequences,
// like '\n', will be interpreted.  NOTE: we make no attempt to
// support '$', i.e. variable substitution, within double quoted strings.
//
// The parsed string content is placed into V starting at V[Vlen], appending
// to an accumulating value in a '.' concatenation sequence.
//
// Returns the total new valid length of the string in V[], or -1 on error.
static int parse_value_quoted(
	struct defcon_context *ctx,
	char **sp,
	char *V,			// OUT: will fill V[Vlen] ff.
	int Vlen,			// length so far
	char *kind,
	int quote
) {
	int i, j;
	int extraline = 0;

	for (i = Vlen; **sp && **sp != quote; (*sp)++, i++) {
		if (i >= VALUELEN) {
			PR_ERR(ctx, "%s too long", kind);
			return -1;
		}
		if (**sp != '\\') {
			if ('\n' == (V[i] = **sp))
				extraline++;
			continue;
		}
		// interpret the next character for a backslash escape
		if ((*sp)[1] == '\0')
			goto unterminated;
		char *special = (quote == '\'')
			? "\\\\''"
			: "n\nr\rt\tv\vf\f\\\\\"\"";
		for (j = 0; special[j]; j += 2)
			if ((*sp)[1] == special[j]) {
				V[i] = special[j+1];
				(*sp)++;
				goto continue_outer;
			}
		if (quote == '"') {
			int digit, literal = 0;
			if (-1 < (digit = oct_digit((*sp)[1]))) {
				// an octal literal
				literal = digit;
				(*sp)++;
				if (literal == 0) { // special case \0
					V[i] = 0;
					continue;
				}
				for (j = 0; j < 2; j++) {
					if (0 > (digit = oct_digit((*sp)[1])))
						break;
					literal = 8 * literal + digit;
					(*sp)++;
				}
				V[i] = literal;
				continue;
			}
			if (	(*sp)[1] == 'x'
			     && -1 < (digit = hex_digit((*sp)[2]))) {
				// an hexadecimal literal
				literal = digit;
				(*sp) += 2;
				if (-1 < (digit = hex_digit((*sp)[1]))) {
					literal = 16 * literal + digit;
					(*sp)++;
				}
				V[i] = literal;
				continue;
			}
		}
		// none of the specially interpreted characters - keep the '\'
		V[i] = '\\';
		continue_outer: ;
	}
	V[i] = '\0';
	if (!**sp) {
unterminated:	PR_ERR(ctx, "Unterminated quoted string");
		return -1;
	}
	(*sp)++;
	if (quote == '`')
		i = replace_shellcommand(ctx, V, Vlen, i-Vlen);
	ctx->line += extraline;
	return i;
}

// Parse an unquoted string value, stopping at whitespace, or at '.' when
// concatenation is permitted, or at separator characters (',' or ';').
//
// If the new unquoted string value happens to be an already defined
// constant, the value of that constant (as a string), is used instead
// of the constant name.
//
// The parsed string content is placed into V starting at V[Vlen], appending
// to an accumulating value in a '.' concatenation sequence.
//
// Returns the total new valid length of the string in V[], or -1 on error.
static int parse_value_unquoted(
	struct defcon_context *ctx,
	char **sp,
	char *V,			// OUT: will fill V[Vlen] ff.
	int Vlen,			// length so far
	char *kind,
	int may_concat
) {
	int i;
	int extraline = 0;

	for (i = Vlen; **sp && !SEP(**sp) && !WS(**sp); (*sp)++, i++) {
		if (may_concat && **sp == '.')
			break;
		if (i >= VALUELEN) {
			PR_ERR(ctx, "%s too long", kind);
			return -1;
		}
		V[i] = **sp;
	}
	V[i] = '\0';
	if (i == 0) {
		PR_ERR(ctx, "No %s found at '%c'", kind, **sp);
		return -1;
	}
	return replace_constant(ctx, V, Vlen, i-Vlen);
}

// Parse a value, either quoted or unquoted, by fanning out work
// to parse_value_quoted() or parse_value_unquoted().
//
// The parsed string content is placed into V starting at V[Vlen], appending
// to an accumulating value in a '.' concatenation sequence.
//
// Returns the total new valid length of the string in V[], or -1 on error
static int parse_value(
	struct defcon_context *ctx,
	enum defcon_keyword_id KW,
	char **sp,
	char *V,			// OUT: will fill V[Vlen] ff.
	int Vlen,			// length so far
	char *kind
) {
	int quote = QUOTE(**sp) ? *((*sp)++) : '\0';

	return quote
		? parse_value_quoted(ctx, sp, V, Vlen, kind, quote)
		: parse_value_unquoted(ctx, sp, V, Vlen, kind,
					keywords[KW].may_concat);
}

static int config_read(
	struct defcon_context *oldctx,
	enum defcon_keyword_id KW,
	char *file
TSRMLS_DC);

static int config_parse(
	struct defcon_context *ctx,
	char *s
TSRMLS_DC) {
	char kw[KEYWORDLEN + 1], N[NAMELEN + 1], V[VALUELEN + 1];
	int Vlen;
	int do_def, i, j;
	char c, *ps;
	enum defcon_keyword_id KW;
	enum defcon_state_id state = ST_KEYWORD;

// helper macro for state transition
#define TRANSIT(NEWSTATE, FMT, ...) do { \
	PR_DBG(ctx, "defcon(%i->%i)" FMT "\n", \
		 state, NEWSTATE, ## __VA_ARGS__); \
	state = NEWSTATE; \
} while (0)
	
	ps = NULL;
	while (*s) {
		if (s == ps) { // avoid endless loops due to programming error
			PR_ERR(ctx, "NO PROGRESS");
			return 0;
		}
		ps = s;

		for (; WS(*s); s++, ps++) {
			if(*s == '\n') {
				// make newline work like ';'
				if (state == ST_CONST_TERM)
					goto const_term;
				if (state == ST_REQUIRE_TERM)
					goto require_term;
				ctx->line++;
			}
		}

		if (!*s)
			break;

		if (*s == '#') {
			for(; *s && *s != '\n'; s++);
			continue;
		}

		switch (state) {
		   case ST_KEYWORD:
			if (KW_INVALID == (KW = parse_keyword(ctx, &s, kw)))
				return 0;
			Vlen = 0;
			TRANSIT(keywords[KW].state, " KW %s", kw);
			break;
		   case ST_CONST_NAME:
			if (0 > (do_def = parse_constantname(ctx, &s, N)))
				return 0;
			TRANSIT(ST_CONST_EQUAL, " name %s", N);
			break;
		   case ST_CONST_EQUAL:
			if (*s != '=') {
				PR_ERR(ctx, "Strange input '%c' ('=' required)",
					*s);
				return 0;
			}
			TRANSIT(ST_CONST_VALUE, " on equal sign");
			s++;
			break;
		   case ST_CONST_VALUE:
			Vlen = parse_value(ctx, KW, &s, V, Vlen, "Value");
			if (0 > Vlen)
				return 0;
			TRANSIT(ST_CONST_TERM, " value '%.*s'", Vlen, V);
			break;
		   case ST_CONST_TERM:
			if (*s == ',') {
				TRANSIT(ST_CONST_NAME, " comma");
			} else if (*s == '.') {
				TRANSIT(ST_CONST_VALUE, " dot");
				s++;
				break;
			} else if (*s == ';') {
// NOTE: we can enter here from the top of the loop, with *s == '\n',
// or from down below, with *s == '\0'.
const_term:			TRANSIT(ST_KEYWORD, " semicolon");
			} else {
				PR_ERR(ctx, "Invalid '%c'", *s);
				return 0;
			}
			if (do_def)
				add_constant(ctx, KW, N, V, Vlen TSRMLS_CC);
			Vlen = 0;
			if (*s == '\n')
				ctx->line++;
			if (*s != '\0')
				s++;
			break;
		   case ST_REQUIRE_PATH: // include/require pathname
			Vlen = parse_value(ctx, KW, &s, V, Vlen, "Pathname");
			if (0 > Vlen)
				return 0;
			TRANSIT(ST_REQUIRE_TERM, "path '%.*s'", Vlen, V);
			break;
		   case ST_REQUIRE_TERM: // after include/require
			if (*s == ',') {
				TRANSIT(ST_REQUIRE_PATH, " comma");
			} else if (*s == '.') {
				TRANSIT(ST_REQUIRE_PATH, " dot");
				s++;
				break;
			} else if (*s == ';') {
// NOTE: we can enter here from the top of the loop, with *s == '\n',
// or from down below, with *s == '\0'.
require_term:			TRANSIT(ST_KEYWORD, " semicolon");
			} else {
				PR_ERR(ctx, "Invalid '%c'", *s);
				return 0;
			}
			if (	!config_read(ctx, KW, V TSRMLS_CC)
			     && KW == KW_REQUIRE)
				return 0;
			Vlen = 0;
			if (*s == '\n')
				ctx->line++;
			if (*s != '\0')
				s++;
			break;
		}
	}

	switch (state) {
		case ST_KEYWORD:
			break;
		case ST_CONST_TERM:
			goto const_term;
		case ST_REQUIRE_TERM:
			goto require_term;
		default:
			PR_ERR(ctx, "Input ends while in state %d", state);
			return 0;
	}

	return 1;
}

static inline int read_dir_order(
	const void *a,
	const void *b
) {
	return strcmp(*((char **) a), *((char **) b));
}

static int config_read_dir(
	struct defcon_context *ctx,
	enum defcon_keyword_id KW
TSRMLS_DC) {
	struct dirent *de, *dep;
	DIR *dir = opendir(ctx->file);
	int len, res;
	char *file, **work;
	int i, n_work;

	if (!dir)
		goto error;

	// see "man 3 readdir" regarding the reentrant readdir_r and
	// how to allocate its arguments.
	de = emalloc(offsetof(struct dirent, d_name)
		     + pathconf(ctx->file, _PC_NAME_MAX)
		     + 1);
	n_work = 0;
	work = emalloc(sizeof(*work));
	// pass 1 - read directory and remember all files matching .conf
	while (0 == readdir_r(dir, de, &dep)) {
		if (!dep)
			break;
		len = strlen(de->d_name);
		if (len < 6 || 0 != strcmp(".conf", de->d_name + len - 5))
			continue;
		work[n_work++] = estrdup(de->d_name);
		work = erealloc(work, (n_work+1) * sizeof(*work));
	}
	efree(de);
	// pass 2 - sort names and then use them
	qsort(work, n_work, sizeof(*work), read_dir_order);
	for (res = 1, i = 0; i < n_work; i++) {
		if (res) {
			len = strlen(work[i]);
			file = emalloc(strlen(ctx->file) + 1 + len + 1);
			sprintf(file, "%s/%s", ctx->file, work[i]);
			if (	!config_read(ctx, KW, file TSRMLS_CC)
			     && KW == KW_REQUIRE)
				res = 0;
			efree(file);
		}
		efree(work[i]);
	}
	efree(work);
	return res;

error:
	if (KW != KW_INCLUDE) { // require or toplevel
		PR_ERR(ctx, "Cannot open directory for reading");
	} else {
		PR_DBG(ctx, "Cannot open directory for reading\n");
	}
	return 0;
}

static int config_read(
	struct defcon_context *oldctx,
	enum defcon_keyword_id KW,
	char *file
TSRMLS_DC) {
	FILE *fd;
	struct stat st;
	int res;
	struct defcon_context ctx[1];

	ctx->module_number = oldctx->module_number;
	ctx->file = file;
	ctx->line = 1;

	if (0 > stat(ctx->file, &st))
		goto error;

	if (S_ISDIR(st.st_mode))
		return config_read_dir(ctx, KW TSRMLS_CC);

	if (!(fd = VCWD_FOPEN(ctx->file, "r")))
		goto error;

	if (!st.st_size) {
		fclose(fd);
		PR_WARN(ctx, "file is empty");
		return 0;
	}

	char *str = emalloc(st.st_size + 1);

	size_t slen = fread(str, 1, st.st_size, fd);
	if (slen < 1) {
		efree(str);
		fclose(fd);
		PR_WARN(ctx, "file is empty");
		return 0;
	}
	str[slen] = '\0';
	fclose(fd);

	res = config_parse(ctx, str TSRMLS_CC);

	efree(str);

	if (!res)
		PR_DBG(ctx, "PARSING ERROR\n");
	return res;

error:	if (KW != KW_INCLUDE) {	// require or toplevel
		PR_ERR(ctx, "Cannot open for reading");
	} else {
		PR_DBG(ctx, "Cannot open for reading\n");
	}
	return 0;
}

PHP_MINIT_FUNCTION(defcon) {
	REGISTER_INI_ENTRIES();

	struct defcon_context ctx[1];
	ctx->module_number = module_number;
	char *file = INI_STR("defcon.config-file");

	if(NULL == file || 0 == strcmp(file, "")) {
		php_error(E_WARNING, "defcon: No Configfile set...");
		return SUCCESS;
	}

	config_read(ctx, KW_INVALID, file TSRMLS_CC);

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(defcon) {
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

