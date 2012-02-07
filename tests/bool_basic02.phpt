--TEST--
bool value handling
--INI--
defcon.config-file = tests/bool_basic02.conf
--FILE--
<?php
echo "FALSEHOOD: '" . FALSEHOOD . "'\n";
?>
--EXPECT--
FALSEHOOD: ''
