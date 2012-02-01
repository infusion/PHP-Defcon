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

static function_entry defcon_functions[] = {
	{NULL, NULL, NULL}
};

PHP_INI_BEGIN()
PHP_INI_ENTRY("defcon.config-file", "/etc/defcon.conf", PHP_INI_ALL, NULL )
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
	const char *g
) {
	int i;
	for (i = 0; i < NR_KW; i++) {
		if (0 == strcasecmp(keywords[i].name, g))
		return i;
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
#if DEBUG_OUTPUT
#define PR_DBG(CTX, FMT, ...) \
	fprintf(stderr, "DEFCON %s:%i " FMT, \
		(CTX)->file, (CTX)->line, ## __VA_ARGS__)
#else
#define PR_DBG(CTX, FMT, ...) do {} while (0)
#endif

static int add_constant(
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
		PR_ERR(ctx, "Constant '%s' redefined", N);

	PR_DBG(ctx, "define('%s', '%.*s')\n", N, Vlen, V);
	return 1;
}

// given a string in V[Vlen+Nlen], use the substring starting at V[Vlen]
// to look up an already defined constant, and if it is already defined,
// replace the string in V[Vlen...Vlen+Nlen] with that constant's value.
// Returns the new overall length of the string in V[], whether replacement
// was done, or not.
int replace_constant(
	struct defcon_context *ctx,
	char *V,
	int Vlen,
	int Nlen
) {
	zval *Z;
	int newlen;

	PR_DBG(ctx, "replace_constant('%.*s') Vlen=%d\n", Nlen, V+Vlen, Vlen);
	if (zend_hash_find(EG(zend_constants), V+Vlen, Nlen+1, (void **)&Z) == SUCCESS) {
		PR_DBG(ctx, "replace_constant('%.*s') Vlen=%d FOUND 0x%p\n", Nlen, V+Vlen, Vlen, Z);
		convert_to_string_ex(&Z);
		PR_DBG(ctx, "replace_constant('%.*s') Vlen=%d CONVERTED => %d '%.*s'\n", Nlen, V+Vlen, Vlen, Z_STRLEN_PP(&Z), Z_STRLEN_PP(&Z), Z_STRVAL_PP(&Z));
		newlen = Vlen + Z_STRLEN_PP(&Z);
		if (newlen <= VALUELEN) {
			PR_DBG(ctx, "replace_constant('%.*s') => %d '%.*s'\n", Nlen, V+Vlen, newlen, Z_STRLEN_PP(&Z), Z_STRVAL_PP(&Z));
			memcpy(V+Vlen, Z_STRVAL_PP(&Z), Z_STRLEN_PP(&Z)+1);
			return newlen;
		}
		PR_DBG(ctx, "replace_constant('%.*s') => %d TOO LONG\n", Nlen, V+Vlen, Vlen+Nlen);
	} else {
		PR_DBG(ctx, "replace_constant('%.*s') => %d NOT FOUND\n", Nlen, V+Vlen, Vlen+Nlen);
	}
	return Vlen+Nlen;
}

#define WS(C) (C == ' ' || C == '\n' || C == '\t' || C == '\r')
#define SEP(C) (C == ',' || C == ';')
#define ALPHA(C) ((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z'))
#define ALNUM(C) (ALPHA(C) || (C >= '0' && C <= '9') || C == '_')

static int parse_keyword(
	struct defcon_context *ctx,
	char **sp,
	char *kw
) {
	int i;
	for (i = 0; ALPHA(**sp); (*sp)++, i++) {
		if (i > KEYWORDLEN) {
			PR_ERR(ctx, "Keyword too long");
			return 0;
		}
		kw[i] = **sp;
	}
	kw[i] = '\0';
	return i;
}

static int parse_constantname(
	struct defcon_context *ctx,
	char **sp,
	char *N
) {
	int i;

	for (i = 0; ALNUM(**sp); (*sp)++, i++) {
		if (i > NAMELEN) {
			PR_ERR(ctx, "Constant name too long");
			return 0;
		}
		N[i] = **sp;
	}
	N[i] = '\0';
	if (i == 0) {
		PR_ERR(ctx, "No Constant name set");
		return 0;
	}

	return i;
}

// returns the total new valid length of the string in V[]
static int parse_value(
	struct defcon_context *ctx,
	enum defcon_keyword_id KW,
	char **sp,
	char *V,			// OUT: will fill V[Vlen] ff.
	int Vlen,			// length so far
	char *kind
) {
	int i, quote = (**sp == '"' || **sp == '\'') ? *((*sp)++) : '\0';
	int extraline = 0;
	int may_concat = !quote && keywords[KW].may_concat;

	PR_DBG(ctx, "parse_value<%s> Vlen=%d quote=%s may_concat=%s nextchar=0x%02x '%c'\n", kind, Vlen, quote ? "true" : "false", may_concat ? "true" : "false", **sp, **sp);

	for (i = Vlen;
	    (quote && **sp && **sp != quote)
	    || (!quote && **sp && !SEP(**sp) && !WS(**sp));
	    (*sp)++, i++) {
		if (may_concat && **sp == '.')
			break;
		if (i > VALUELEN) {
			PR_ERR(ctx, "%s too long", kind);
			return -1;
		}
		V[i] = **sp;
		if (quote && **sp == '\n')
			extraline++;
	}

	V[i] = '\0';

	if (quote) {
		if (!**sp) {
			PR_ERR(ctx, "Unterminated quoted string");
			return -1;
		}
		(*sp)++;
		ctx->line += extraline;
	} else {
		if (i == 0) {
			PR_ERR(ctx, "No %s found at '%c'", kind, **sp);
			return -1;
		}
		i = replace_constant(ctx, V, Vlen, i-Vlen);
	}
	PR_DBG(ctx, "parse_value => %d '%.*s'\n", i, i, V);
	return i;
}

static int config_read(
	struct defcon_context *ctx,
	enum defcon_keyword_id KW
TSRMLS_DC);

static int config_parse(
	struct defcon_context *ctx,
	char *s
TSRMLS_DC) {
	char kw[KEYWORDLEN + 1], N[NAMELEN + 1], V[VALUELEN + 1];
	int Vlen;
	int i, j;
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
			if (0 >= (i = parse_keyword(ctx, &s, kw)))
				return 0;

			KW = match_keyword(kw);
			if (KW_INVALID == KW) {
				PR_ERR(ctx, "No valid keyword (%s)", kw);
				return 0;
			}
			Vlen = 0;
			TRANSIT(keywords[KW].state, " KW %.*s", i, kw);
			break;
		   case ST_CONST_NAME:
			if (0 >= (i = parse_constantname(ctx, &s, N)))
				return 0;

			if (KW_INVALID != match_keyword(N)) {
				PR_ERR(ctx, "Constant name '%s' should"
					    " not be a keyword", N);
				return 0;
			}

			TRANSIT(ST_CONST_EQUAL, " name %.*s", i, N);
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
				TRANSIT(ST_CONST_VALUE, " period");
				s++;
				break;
			} else if (*s == ';') {
const_term:			TRANSIT(ST_KEYWORD, " semicolon");
			} else {
				PR_ERR(ctx, "Invalid '%c'", *s);
				return 0;
			}
			if (!add_constant(ctx, KW, N, V, Vlen TSRMLS_CC))
				return 0;
			Vlen = 0;
			if (*s == '\n')
				ctx->line++;
			if (*s)
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
				TRANSIT(ST_REQUIRE_PATH, " period");
				s++;
				break;
			} else if (*s == ';') {
require_term:			TRANSIT(ST_KEYWORD, " semicolon");
			} else {
				PR_ERR(ctx, "Invalid '%c'", *s);
				return 0;
			}
			struct defcon_context Nctx[1];
			Nctx->module_number = ctx->module_number;
			Nctx->file = V;
			Nctx->line = 1;
			if (	!config_read(Nctx, KW TSRMLS_CC)
			     && KW == KW_REQUIRE)
				return 0;
			Vlen = 0;
			if (*s == '\n')
				ctx->line++;
			if (*s)
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

static int config_read_dir(
	struct defcon_context *ctx,
	enum defcon_keyword_id KW
TSRMLS_DC) {
	struct dirent *de, *dep;
	DIR *dir = opendir(ctx->file);
	int len, res;
	struct defcon_context Nctx[1];

	if (!dir)
		goto error;

	// see "man 3 readdir" regarding the reentrant readdir_r and
	// how to allocate its arguments.
	de = emalloc(offsetof(struct dirent, d_name)
		     + pathconf(ctx->file, _PC_NAME_MAX)
		     + 1);
	res = 1;
	while (res && 0 == readdir_r(dir, de, &dep)) {
		if (!dep)
			break;
		len = strlen(de->d_name);
		if (len < 6 || 0 != strcmp(".conf", de->d_name + len - 5))
			continue;
		Nctx->module_number = ctx->module_number;
		Nctx->file = emalloc(strlen(ctx->file) + 1 + len + 1);
		Nctx->line = 1;
		sprintf(Nctx->file, "%s/%s", ctx->file, de->d_name);
		if (!config_read(Nctx, KW TSRMLS_CC) && KW == KW_REQUIRE)
			res = 0;
		efree(Nctx->file);
	}
	efree(de);
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
	struct defcon_context *ctx,
	enum defcon_keyword_id KW
TSRMLS_DC) {
	FILE *fd;
	struct stat st;
	int res;

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
	ctx->file = INI_STR("defcon.config-file");
	ctx->line = 1;

	if(NULL == ctx->file || 0 == strcmp(ctx->file, "")) {
		php_error(E_WARNING, "defcon: No Configfile set...");
		return SUCCESS;
	}

	config_read(ctx, KW_INVALID TSRMLS_CC);

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(defcon) {
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

