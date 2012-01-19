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

	# Parsertest
	int T1 = 123;
	int T2 = 1.3;
	int T3 = "test";
	real T4 = 1;

	string T5 = hello # should work too

	logical T6 = true;
	logical T7 = false;
	logical T8 = 234;

	require '/var/www/vhosts/example.com/defcon.conf'

Just add this line to your php.ini file:

	defcon.config-file = "/etc/defcon.conf"

You could also include an entire directory like this:

	defcon.config-file = "/etc/defcon.conf.d"


Installation
============

	* Download defcon source package
	* Unpack defcon source package
	* Go to defcon folder and type "phpize && ./configure && make && make install"
	* Make sure you have extension=defcon.so in your php.ini
	* Add the configuration lines from above in your /etc/defcon.conf

