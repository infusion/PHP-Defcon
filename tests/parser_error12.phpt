--TEST--
Fail unterminated quoted string
--INI--
defcon.config-file = tests/parser_error12.conf
error_log = /dev/null
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error12.conf line 7: Unterminated quoted string in Unknown on line 0
OK
