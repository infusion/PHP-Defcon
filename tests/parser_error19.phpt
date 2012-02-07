--TEST--
Fail constant names starting with a digit
--INI--
defcon.config-file = tests/parser_error19.conf
error_log = /dev/null
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error19.conf line 8: No Constant name set in Unknown on line 0
OK
