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
};

static struct defcon_keyword keywords[] = {
/*  0 */	{ "string" },
/*  1 */	{ "int" },
/*  2 */	{ "long" },
/*  3 */	{ "float" },
/*  4 */	{ "real" },
/*  5 */	{ "double" },
/*  6 */	{ "bool" },
/*  7 */	{ "boolean" },
/*  8 */	{ "logical" },
/*  9 */	{ "short" },
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

static int config_read(struct defcon_context *ctx TSRMLS_DC);

static int config_parse(struct defcon_context *ctx, char *s TSRMLS_DC) {
	char kw[KEYWORDLEN + 1], N[NAMELEN + 1], V[VALUELEN + 1];
	int i;
	char c;
	short KW, state = 0;

	for(; *s; s++) {

		for(; *s && (*s == ' ' || *s == '\n' || *s == '\t' || *s == '\r'); s++) {
			if(*s == '\n') ctx->line++;
		}

		if(!*s)
		   break;

		if(*s == '#') {
			for(; *s && *s != '\n'; s++);
		} else {

			if(*s == ',')
			  state = 4;
			else if(*s == ';')
			  state = 5;

			switch(state) {
			   case 0: // catch keyword
				for(i = 0; *s && (*s >= 'a' && *s <= 'z' || *s >= 'A' && *s <= 'Z'); s++, i++) {
					if(i > KEYWORDLEN) {
						PR_ERR(ctx, "Typ too long");
						return 0;
					}
					kw[i] = *s;
				}
				kw[i] = '\0';
				KW = match_keyword(kw);
				if(-1 == KW) {
					PR_ERR(ctx, "No valid KW (%s)", kw);
					return 0;
				}
				state = 1;
				break;
			   case 1: // catch varname
				for(i = 0; *s >= 'a' && *s <= 'z' || *s >= 'A' && *s <= 'Z' || *s >= '0' && *s <= '9' || *s == '_'; s++, i++) {
					if(i > NAMELEN) {
						PR_ERR(ctx, "Varname too long");
						return 0;
					}
					N[i] = *s;
				}
				N[i] = '\0';
				if(N[0] == '\0') {
					PR_ERR(ctx, "No Varname set");
					return 0;
				}

				if(-1 != match_keyword(V)) {
					PR_ERR(ctx, "Varname should not be a keyword");
					return 0;
				}

				state = 2;
				s--;
				break;
				case 2: // catch =
				if(*s != '=') {
					PR_ERR(ctx, "Strange input ( = required)", *s);
					return 0;
				}
				state = 3;
				break;
			   case 3: // final state
				if(*s == '"' || *s == '\'') c = *(s++);
				else c = '\0';;

				for(i=0; *s && (c && *s != c || (!c && (*s >= '0' && *s<='9' || *s >= 'a' && *s<='z' || *s >= 'A' && *s<='Z' || *s == '.'))); s++, i++) {
					if(i > VALUELEN) {
						PR_ERR(ctx, "Value too long");
						return 0;
					}
					V[i] = *s;
				}

				V[i] = '\0';

				if(c) s++;
				else if(V[0] == '\0') {
					PR_ERR(ctx, "No Value found");
					return 0;

				}
				if (!add_constant(ctx, KW, N, V TSRMLS_CC))
					return 0;

				state = 0;
				break;
			   case 4: // , found
				state = 1;
				break;
			   case 5: // ; found
				state = 0;
				break;

			}
		}
	}
	return 1;
}

static int config_read(struct defcon_context *ctx TSRMLS_DC)
{
	FILE *fd;
	struct stat st;
	int res;

	if (0 > stat(ctx->file, &st) || !(fd = VCWD_FOPEN(ctx->file, "r"))) {
		PR_ERR(ctx, "Cannot open for reading");
		return 0;
	}

	if (!st.st_size) {
		PR_WARN(ctx, "file is empty");
		return 0;
	}

	char *str = emalloc(st.st_size + 1);

	size_t slen = fread(str, 1, st.st_size, fd);
	if (slen < 1) {
		efree(str);
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

	config_read(ctx TSRMLS_CC);

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(defcon) {
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

