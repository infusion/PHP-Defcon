--TEST--
single quoted string parsing
--INI--
defcon.config-file = tests/quoted_single_basic01.conf
--FILE--
<?php
echo DEFCON_STRING_1 . "\n";
?>
--EXPECT--
single quoted
