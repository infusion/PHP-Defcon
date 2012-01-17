--TEST--
Value termination / line count parser error handling
--INI--
defcon.config-file = tests/parser_error11.conf
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error11.conf line 7: No valid keyword () in Unknown on line 0
OK
