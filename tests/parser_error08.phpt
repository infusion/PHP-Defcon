--TEST--
Fail overly long value
--INI--
defcon.config-file = tests/parser_error08.conf
error_log = /dev/null
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error08.conf line 7: Value too long in Unknown on line 0
OK
