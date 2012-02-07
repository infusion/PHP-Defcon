--TEST--
Fail on-the-boundary overly long value
--INI--
defcon.config-file = tests/parser_error16.conf
error_log = /dev/null
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error16.conf line 7: Value too long in Unknown on line 0
OK
