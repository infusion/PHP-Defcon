--TEST--
good_basic.conf configuration reading
--INI--
defcon.config-file = tests/good_basic.conf
--FILE--
<?php
echo DEFCON_GOOD_STRING . "\n";
echo DEFCON_GOOD_INT . "\n";
echo DEFCON_GOOD_FLOAT . "\n";
echo DEFCON_GOOD_BOOL . "\n";
?>
--EXPECT--
Good String Content
23
42.23
1
