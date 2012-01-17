--TEST--
Unterminated quoted string parser error handling
--INI--
defcon.config-file = tests/parser_error12.conf
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error12.conf line 7: Unterminated quoted string in Unknown on line 0
OK
