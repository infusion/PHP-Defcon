--TEST--
double quoted string parsing
--INI--
defcon.config-file = tests/quoted_double_basic03.conf
--FILE--
<?php
echo DEFCON_STRING_1 . "\n";
?>
--EXPECT--
double \p
quoted	with newline and tab
