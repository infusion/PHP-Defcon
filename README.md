PHP Defcon Extension
====================

PHP Defcon is an PHP extension to add constants at start time of the engine for the entire runtime.
Normally, you would suppose to add config constants to your PHP Script for every request like this:

	define('DB_USER', 'root');
	define('DB_PASS', 'MyS3cR37PW!');
	define('DB_NAME', 'test');

	define('F_IS_FRIEND', 1);
	define('F_IS_FOF', 2);

A Better alternative would be using Defcon. An Example config file could look like this:

	# Database Config
	string DB_HOST = "localhost";
	string DB_USER = "root";
	string DB_PASS = "yS3cR37PW!";
	string DB_NAME = "test";

	# Store the eth0 IP into a constant using the backtick operator
	string HOST_IP = `ifconfig eth0 | sed -n 's/ *inet addr:\([0-9.]\+\).*/\1/p'`

	# Parsertest
	int T1 = 123;
	int T2 = 1.3;
	int T3 = "test";
	real T4 = 1;

	string T5 = hello # should work too

	logical T6 = true;
	logical T7 = false;
	logical T8 = 234;

	require '/var/admin/defcon.global.conf'

Config File Reading
-------------------

The *require* and *include* keywords can be used to include an additional
file from a given config file. *include* ignores errors, while *require*
stops the defcon configuration process on error. **NOTE** put single or double
quotes around the pathnames, unless they do not contain a dot '.' character.

To change the config file from the default **/etc/defcon.conf**, add to php.ini:

	defcon.config-file = "/some/where/else.conf"

If the config file given, either in php.ini or in a require/include
statement, really happens to be a **directory**, all files from that
directory with names ending in **.conf** will be read, in sorted order.
If any of these names is again a directory, that other directory
will be read, recursively.

Constant Value Syntax
---------------------

You can write the constant values, after the '=' sign, either surrounded
by quotes, or without any quotes.

If you write values **without any quotes**, the value will end as soon as some
syntactically relevant character is encountered, like whitespace, newline,
comma, semicolon, and sometimes a dot. **Furthermore** if such an unquoted
value happens to be the name of an **already defined constant**, the value
of that constant is **substituted** in its place.

Quoted strings work almost like in PHP itself:

* Single quoted strings permit backslash escapes for embedding a single quote,
  or a backslash itself; all other backslashes are left alone.
* Double quoted strings permit the full set of backslash escapes.
  However, no effort is made to implement Dollar (Variable) substitution!
* Backtick quoted strings first interpret the content like double quoted
  strings, wrt. backslash escapes, and then run the result as a shell
  command, substituting the command output.

For *string* constants, as well as the pathes of *require* and *include*
statements, you can additionally use the dot '.' character to concatenate
multiple strings together. This is especially useful together with the
aforementioned replacement of *unquoted* values that happen to be known
constants, like in the following example:

	string PREFIX = "/usr/local"
	string BIN = PREFIX . "/bin"

**NOTE:** for any single constant assignment, the *overall* length of
the value is restricted to a compile time constant, 4096 byte. This is
usually warned about when it happens, except for backtick command
substitution, where the command output is silently truncated.

Installation
============

* Download defcon source package
* Unpack defcon source package
* Go to defcon folder and type "phpize && ./configure && make"
* Maybe run the included test cases:
	make test PHP_EXECUTABLE=/usr/bin/php5 TEST_PHP_ARGS="-q"
* If all is well, run "make install", as root, to copy modules/defcon.so
  to your PHP extension directory.
* Make sure you have extension=defcon.so in your php.ini, or in a file
  like /etc/php5/conf.d/defcon.ini
* Add configuration to your /etc/defcon.conf, or to whatever file
  you configured using the defcon.config-file= php.ini setting.

