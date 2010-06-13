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

#ifndef PHP_DEFCON_H
#define PHP_DEFCON_H

#define PHP_DEFCON_VERSION "1.0"
#define PHP_DEFCON_EXTNAME "defcon"

#define TYPELEN 64
#define NAMELEN 64
#define VALUELEN 128

extern zend_module_entry defcon_module_entry;
#define phpext_defcon_ptr &defcon_module_entry

#ifdef ZTS
# include "TSRM.h"
#endif

PHP_MINFO_FUNCTION(defcon);
PHP_MINIT_FUNCTION(defcon);
PHP_MSHUTDOWN_FUNCTION(defcon);

#endif

