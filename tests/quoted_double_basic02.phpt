--TEST--
double quoted string parsing
--INI--
defcon.config-file = tests/quoted_double_basic02.conf
--FILE--
<?php
echo DEFCON_STRING_1 . "\n";
echo DEFCON_STRING_2 . "\n";
echo DEFCON_STRING_3 . "\n";
echo DEFCON_STRING_4 . "\n";
?>
--EXPECT--
double\quoted
double"quoted
double quoted"
double quoted\'
