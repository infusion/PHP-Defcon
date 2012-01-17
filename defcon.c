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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_defcon.h"

static const char *dt[] = {
	"string",	/* 0 */
	"int",		/* 1 */
	"long",		/* 2 */
	"float",	/* 3 */
	"real",		/* 4 */
	"double",	/* 5 */
	"bool",		/* 6 */
	"boolean",	/* 7 */
	"logical",	/* 8 */
	"short",	/* 9 */
	NULL
};


static short match_type(const char *g TSRMLS_DC) {
	int i=0;
	for(; dt[i] != NULL; i++) {
		if(0 == strcasecmp(dt[i], g))
		return i;
	}
	return -1;
}

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

PHP_MINIT_FUNCTION(defcon) {
	REGISTER_INI_ENTRIES();

	FILE *fd;
	char *file = INI_STR("defcon.config-file");
	char typ[TYPELEN + 1], N[NAMELEN + 1], V[VALUELEN + 1];
	int i, j, line = 1;
	char c;
	short T, state = 0;
	struct stat st;

	if(NULL == file || 0 == strcmp(file, "")) {
		php_error(E_WARNING, "defcon: No Configfile set...");
		return SUCCESS;
	}

	if(-1 != stat(file, &st) && NULL != (fd = VCWD_FOPEN(file, "r"))) {

		if(!st.st_size) {
			php_error(E_WARNING, "defcon: Configfile empty");
			return SUCCESS;
		}

		char *s, *str = emalloc(st.st_size + 1);
		s = str;

		for(i=0, c=0; c != EOF; i++) {
			c = fgetc(fd);
			s[i] = c;
		}
		s[i-2] = '\0';
		fclose(fd);


		for(; *s; s++) {

			for(; *s && (*s == ' ' || *s == '\n' || *s == '\t' || *s == '\r'); s++) {
				if(*s == '\n') line++;
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
				   case 0: // catch type
					for(i = 0; *s && (*s >= 'a' && *s <= 'z' || *s >= 'A' && *s <= 'Z'); s++, i++) {
						if(i > TYPELEN) {
							php_error(E_ERROR, "defcon: Typ too long on line %i", line);
							efree(str);
							return SUCCESS;
						}
						typ[i] = *s;
					}
					typ[i] = '\0';
					T = match_type(typ TSRMLS_DC);
					if(-1 == T) {
						php_error(E_ERROR, "defcon: No valid Typ given on line %i (%s)", line, typ);
						efree(str);
						return SUCCESS;
					}
					state = 1;
					break;
				   case 1: // catch varname
					for(i = 0; *s >= 'a' && *s <= 'z' || *s >= 'A' && *s <= 'Z' || *s >= '0' && *s <= '9' || *s == '_'; s++, i++) {
						if(i > NAMELEN) {
							php_error(E_ERROR, "defcon: Varname too long on line %i", line);
							efree(str);
							return SUCCESS;
						}
						N[i] = *s;
					}
					N[i] = '\0';
					if(N[0] == '\0') {
						php_error(E_ERROR, "defcon: No Varname set on line %i", line);
						efree(str);
						return SUCCESS;
					}

					if(-1 != match_type(V TSRMLS_DC)) {
						php_error(E_ERROR, "defcon: Varname should not be a type on line %i", line);
						efree(str);
						return SUCCESS;
					}

					state = 2;
					s--;
					break;
					case 2: // catch =
					if(*s != '=') {
						php_error(E_ERROR, "defcon: Strange input on line %i ( = required)", *s, line);
						efree(str);
						return SUCCESS;
					}
					state = 3;
					break;
				   case 3: // final state
					if(*s == '"' || *s == '\'') c = *(s++);
					else c = '\0';;
	
					for(i=0; *s && (c && *s != c || (!c && (*s >= '0' && *s<='9' || *s >= 'a' && *s<='z' || *s >= 'A' && *s<='Z' || *s == '.'))); s++, i++) {
						if(i > VALUELEN) {
							php_error(E_ERROR, "defcon: Value too long on line %i", line);
							efree(str);
							return SUCCESS;
						}
						V[i] = *s;
					}

					V[i] = '\0';

					if(c) s++;
					else if(V[0] == '\0') {
						php_error(E_ERROR, "defcon: No Value found on line %i", line);
						efree(str);
						return SUCCESS;
	
					}
	
					zend_constant zc;
	
					switch(T) {
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
						s--;
						break;
					   case 3:
					   case 4:
					   case 5:
						zc.value.type = IS_DOUBLE;
						zc.value.value.dval = atof(V);
						s--;
						break;
					   case 6:
					   case 7:
					   case 8:
						zc.value.type = IS_BOOL;
						if(0 == strcasecmp(V, "true")) {
							zc.value.value.lval = 1;
						} else if(0 == strcasecmp(V, "false")) {
							zc.value.value.lval = 0;
						} else {
							zc.value.value.lval = atol(V);
						}
						s--;
						break;
					}
					zc.flags = CONST_CS | CONST_PERSISTENT;
					zc.name = zend_strndup(N, strlen(N));
					zc.name_len = strlen(N)+1;
					zc.module_number = module_number;

					if(zend_register_constant(&zc TSRMLS_CC) == FAILURE) {
						php_error(E_ERROR, "defcon: Constant '%s' redefined on line %i", N, line);
					}
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

		efree(str);

	} else {
		php_error(E_ERROR, "defcon: Can't stat file %s", file);
		return SUCCESS;
	}


	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(defcon) {
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

