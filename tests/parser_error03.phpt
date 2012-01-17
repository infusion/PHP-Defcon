--TEST--
Overly long keyword parser error handling
--INI--
defcon.config-file = tests/parser_error03.conf
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error03.conf line 7: Keyword too long in Unknown on line 0
OK
