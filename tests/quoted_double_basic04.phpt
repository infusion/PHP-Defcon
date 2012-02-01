--TEST--
double quoted string octal escapes
--INI--
defcon.config-file = tests/quoted_double_basic04.conf
--FILE--
<?php
echo DEFCON_STRING_1 . "\n";
?>
--EXPECT--
with octal X - "\130" - here: X
