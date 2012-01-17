--TEST--
No value found parser error handling
--INI--
defcon.config-file = tests/parser_error09.conf
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error09.conf line 2: No Value found at '=' in Unknown on line 0
OK
