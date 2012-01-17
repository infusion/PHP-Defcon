--TEST--
Unknown keyword parser error handling
--INI--
defcon.config-file = tests/parser_error02.conf
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error02.conf line 7: No valid keyword (unknownkeyword) in Unknown on line 0
OK
