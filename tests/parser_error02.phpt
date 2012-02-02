--TEST--
Fail unknown keyword
--INI--
defcon.config-file = tests/parser_error02.conf
error_log = /dev/null
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error02.conf line 7: No valid keyword (unknownkeyword) in Unknown on line 0
OK
