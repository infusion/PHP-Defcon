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

struct defcon_keyword {
	char *name;
	int state;	// state to switch to when found
};

static struct defcon_keyword keywords[] = {
/*  0 */	{ "string",	1 },
/*  1 */	{ "int",	1 },
/*  2 */	{ "long",	1 },
/*  3 */	{ "float",	1 },
/*  4 */	{ "real",	1 },
/*  5 */	{ "double",	1 },
/*  6 */	{ "bool",	1 },
/*  7 */	{ "boolean",	1 },
/*  8 */	{ "logical",	1 },
/*  9 */	{ "short",	1 },
/* 10 */	{ "require",	5 },
/* 11 */	{ "include",	5 },
};
#define NR_KW (sizeof(keywords)/sizeof(keywords[0]))

static short match_keyword(const char *g) {
	int i;
	for (i = 0; i < NR_KW; i++) {
		if (0 == strcasecmp(keywords[i].name, g))
		return i;
	}
	return -1;
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
	short KW,
	char *N,
	char *V
TSRMLS_DC) {
	zend_constant zc;

	switch (KW) {
	   case 0:
		zc.value.type = IS_STRING;
		zc.value.value.str.val = zend_strndup(V, strlen(V));
		zc.value.value.str.len = strlen(V);
		break;
	   case 1:
	   case 2:
	   case 9:
		zc.value.type = IS_LONG;
		zc.value.value.lval = atol(V);
		break;
	   case 3:
	   case 4:
	   case 5:
		zc.value.type = IS_DOUBLE;
		zc.value.value.dval = atof(V);
		break;
	   case 6:
	   case 7:
	   case 8:
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

	return 1;
}

#define WS(C) (C == ' ' || C == '\n' || C == '\t' || C == '\r')
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

static int parse_value(
	struct defcon_context *ctx,
	char **sp,
	char *V,
	char *kind
) {
	int i, quote = (**sp == '"' || **sp == '\'') ? *((*sp)++) : '\0';
	int extraline = 0;

	for (i = 0;
	    (quote && **sp && **sp != quote)
	    || (!quote && **sp && **sp != ',' && **sp != ';' && !WS(**sp));
	    (*sp)++, i++) {
		if (i > VALUELEN) {
			PR_ERR(ctx, "%s too long", kind);
			return 0;
		}
		V[i] = **sp;
		if (quote && **sp == '\n')
			extraline++;
	}

	V[i] = '\0';

	if (quote) {
		if (!**sp) {
			PR_ERR(ctx, "Unterminated quoted string");
			return 0;
		}
		(*sp)++;
		ctx->line += extraline;
	} else if (i == 0) {
		PR_ERR(ctx, "No %s found at '%c'", kind, **sp);
		return 0;
	}

	return i;
}

static int config_read(struct defcon_context *ctx, int KW TSRMLS_DC);

static int config_parse(struct defcon_context *ctx, char *s TSRMLS_DC) {
	char kw[KEYWORDLEN + 1], N[NAMELEN + 1], V[VALUELEN + 1];
	int i, j;
	char c, *ps;
	short KW, maybe_KW;
	short prev_state = 0, state = 0;

// helper macro for state transition
#define TRANSIT(NEWSTATE, FMT, ...) do { \
	PR_DBG(ctx, "defcon(%i->%i)" FMT "\n", \
		 state, NEWSTATE, ## __VA_ARGS__); \
	prev_state = state; \
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
				if (state == 4 || state == 6) {
					// accept newline instead of ',' or ';'
					TRANSIT(0, " at \\n");
				}
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
		   case 0: // catch keyword
			if (0 >= (i = parse_keyword(ctx, &s, kw)))
				return 0;

			KW = match_keyword(kw);
			if (-1 == KW) {
				PR_ERR(ctx, "No valid keyword (%s)", kw);
				return 0;
			}
			TRANSIT(keywords[KW].state, " KW %.*s", i, kw);
			break;
		   case 1: // ENTRY for type keywords: catch varname
			if (0 >= (i = parse_constantname(ctx, &s, N)))
				return 0;

			maybe_KW = match_keyword(N);
			if (-1 != maybe_KW) {
				if (prev_state != 4) { // NOT after comma
					PR_ERR(ctx, "Constant name should"
						    " not be a keyword");
					return 0;
				}
				KW = maybe_KW;
				TRANSIT(keywords[KW].state, " KW %.*s", i, N);
				break;
			}

			TRANSIT(2, " name %.*s", i, N);
			break;
		   case 2: // catch =
			if (*s != '=') {
				PR_ERR(ctx, "Strange input '%c' ('=' required)",
					*s);
				return 0;
			}
			TRANSIT(3, " on equal sign");
			s++;
			break;
		   case 3: // final state - value
			if (0 >= (i = parse_value(ctx, &s, V, "Value")))
				return 0;

			if (!add_constant(ctx, KW, N, V TSRMLS_CC))
				return 0;

			TRANSIT(4, " value '%.*s'", i, V);
			break;
		   case 4: // after a constant definition - see how it goes on
			if (*s == ',') {
				TRANSIT(1, " comma");
				s++;
				break;
			}
			if (*s == ';') {
				TRANSIT(0, " semicolon");
				s++;
				break;
			}
			PR_ERR(ctx, "Invalid '%c'", *s);
			return 0;
		   case 5: // ENTRY for keywords 'include' and 'require'
			if (0 >= (i = parse_value(ctx, &s, V, "Pathname")))
				return 0;

			struct defcon_context Nctx[1];
			Nctx->module_number = ctx->module_number;
			Nctx->file = V;
			Nctx->line = 1;
			if (!config_read(Nctx, KW TSRMLS_CC) && KW == 10)
				return 0;

			TRANSIT(6, "");
			break;
		   case 6: // after include/require - see how it goes on
			if (*s == ',') {
				TRANSIT(5, " comma");
				s++;
				break;
			}
			if (*s == ';') {
				TRANSIT(0, " semicolon");
				s++;
				break;
			}
			PR_ERR(ctx, "Invalid '%c'", *s);
			return 0;
		}
	}

	return 1;
}

static int config_read_dir(struct defcon_context *ctx, int KW TSRMLS_DC)
{
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
		if (!config_read(Nctx, KW TSRMLS_CC) && KW == 10)
			res = 0;
		efree(Nctx->file);
	}
	efree(de);
	return res;

error:
	if (KW != 11) { // require or toplevel
		PR_ERR(ctx, "Cannot open directory for reading");
	} else {
		PR_DBG(ctx, "Cannot open directory for reading\n");
	}
	return 0;
}

static int config_read(struct defcon_context *ctx, int KW TSRMLS_DC)
{
	FILE *fd;
	struct stat st;
	int res;

	if (0 > stat(ctx->file, &st)) {
no_such_file:	if (KW != 11) {	// require or toplevel
			PR_ERR(ctx, "Cannot open for reading");
		} else {
			PR_DBG(ctx, "Cannot open for reading\n");
		}
		return 0;
	}

	if (S_ISDIR(st.st_mode))
		return config_read_dir(ctx, KW TSRMLS_CC);

	if (!(fd = VCWD_FOPEN(ctx->file, "r")))
		goto no_such_file;

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

	config_read(ctx, -1 TSRMLS_CC);

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(defcon) {
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

