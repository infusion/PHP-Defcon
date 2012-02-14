--TEST--
Ignore @shutup constant redefinition
--INI--
defcon.config-file = tests/shutup_basic.conf
error_log = /dev/null
--FILE--
<?php
echo A . " OK\n";
?>
--EXPECT--
a OK
