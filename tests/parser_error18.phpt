--TEST--
Fail on-the-boundary overly long constant name
--INI--
defcon.config-file = tests/parser_error18.conf
error_log = /dev/null
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error18.conf line 8: Constant name too long in Unknown on line 0
OK
