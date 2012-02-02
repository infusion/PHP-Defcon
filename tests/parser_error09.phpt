--TEST--
Fail missing value
--INI--
defcon.config-file = tests/parser_error09.conf
error_log = /dev/null
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error09.conf line 7: No Value found at ';' in Unknown on line 0
OK
