--TEST--
Handle empty configuration file
--INI--
defcon.config-file = tests/parser_error01.conf
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
Warning: defcon: tests/parser_error01.conf line 1: file is empty in Unknown on line 0
OK
