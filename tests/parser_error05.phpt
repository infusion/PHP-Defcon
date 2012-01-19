--TEST--
Fail empty constant name
--INI--
defcon.config-file = tests/parser_error05.conf
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error05.conf line 7: No Constant name set in Unknown on line 0
OK
