--TEST--
double quoted string parsing
--INI--
defcon.config-file = tests/quoted_double_basic01.conf
--FILE--
<?php
echo DEFCON_STRING_1 . "\n";
?>
--EXPECT--
double quoted
