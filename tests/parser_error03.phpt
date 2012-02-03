--TEST--
Fail overly long keyword
--INI--
defcon.config-file = tests/parser_error03.conf
error_log = /dev/null
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error03.conf line 7: Keyword too long in Unknown on line 0
OK
