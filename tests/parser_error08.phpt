--TEST--
Value too long parser error handling
--INI--
defcon.config-file = tests/parser_error08.conf
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error08.conf line 2: Value too long in Unknown on line 0
OK
