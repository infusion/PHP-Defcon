--TEST--
single quoted string parsing
--INI--
defcon.config-file = tests/quoted_single_basic02.conf
--FILE--
<?php
echo DEFCON_STRING_1 . "\n";
echo DEFCON_STRING_2 . "\n";
echo DEFCON_STRING_3 . "\n";
echo DEFCON_STRING_4 . "\n";
?>
--EXPECT--
single\quoted
single'quoted
single quoted'
single quoted\"
