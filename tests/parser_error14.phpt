--TEST--
Fail repeated comma
--INI--
defcon.config-file = tests/parser_error14.conf
error_log = /dev/null
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error14.conf line 7: No Constant name set in Unknown on line 0
OK
