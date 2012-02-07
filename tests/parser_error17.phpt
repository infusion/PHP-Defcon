--TEST--
Fail on-the-boundary overly long keyword
--INI--
defcon.config-file = tests/parser_error17.conf
error_log = /dev/null
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error17.conf line 8: Keyword too long in Unknown on line 0
OK
