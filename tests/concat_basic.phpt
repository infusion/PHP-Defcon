--TEST--
concat_basic.conf test string concatenation
--INI--
defcon.config-file = tests/concat_basic.conf
--FILE--
<?php
echo DEFCON_GOOD_STRING . "\n";
echo DEFCON_ANOTHER_STRING . "\n";
echo DEFCON_THIRD_STRING . "\n";
echo DEFCON_GOOD_INT . "\n";
echo DEFCON_GOOD_FLOAT . "\n";
echo DEFCON_GOOD_BOOL . "\n";
?>
--EXPECT--
Good String Content
Good String Content and more
Good String Content reused
23
42.23
1
