--TEST--
recursive subdirectory require
--INI--
defcon.config-file = tests/require_dir_basic.conf.d
--FILE--
<?php
echo DEFCON_GOOD_STRING . "\n";
echo DEFCON_ANOTHER_STRING . "\n";
echo DEFCON_GOOD_INT . "\n";
echo DEFCON_GOOD_FLOAT . "\n";
echo DEFCON_GOOD_BOOL . "\n";
?>
--EXPECT--
Good String Content
Good String Content and more
23
42.23
1
