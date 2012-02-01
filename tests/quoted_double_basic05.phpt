--TEST--
double quoted string hex escapes
--INI--
defcon.config-file = tests/quoted_double_basic05.conf
--FILE--
<?php
echo DEFCON_STRING_1 . "\n";
echo DEFCON_STRING_2 . "\n";
?>
--EXPECT--
with hex X - "\x58" - here: X
incomplete hex - "\xy" - verbatim: \xy
