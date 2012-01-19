--TEST--
Fail overly long constant name
--INI--
defcon.config-file = tests/parser_error04.conf
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error04.conf line 7: Constant name too long in Unknown on line 0
OK
