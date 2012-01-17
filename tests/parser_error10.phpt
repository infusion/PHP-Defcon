--TEST--
Constant redefinition parser error handling
--INI--
defcon.config-file = tests/parser_error10.conf
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Notice: Constant A already defined in Unknown on line 0

Fatal error: defcon: tests/parser_error10.conf line 7: Constant 'A' redefined in Unknown on line 0
OK
