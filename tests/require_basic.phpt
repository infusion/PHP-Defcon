--TEST--
require file inclusion
--INI--
defcon.config-file = tests/require_basic.conf
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
