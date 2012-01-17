--TEST--
Multiline quoted string line count parser error handling
--INI--
defcon.config-file = tests/parser_error13.conf
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error13.conf line 11: No valid keyword (provokeanerror) in Unknown on line 0
OK
