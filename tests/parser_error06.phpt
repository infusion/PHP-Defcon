--TEST--
Constant name is keyword parser error handling
--INI--
defcon.config-file = tests/parser_error06.conf
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error06.conf line 7: Constant name should not be a keyword in Unknown on line 0
OK
