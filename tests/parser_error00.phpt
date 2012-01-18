--TEST--
Missing configuration file handling
--INI--
defcon.config-file = tests/parser_error00.conf
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Fatal error: defcon: tests/parser_error00.conf line 1: Cannot open for reading in Unknown on line 0
OK
