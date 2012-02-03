--TEST--
Check multiline quoted string line count
--INI--
defcon.config-file = tests/parser_error13.conf
error_log = /dev/null
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error13.conf line 11: No valid keyword (provokeanerror) in Unknown on line 0
OK
