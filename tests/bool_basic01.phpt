--TEST--
bool value handling
--INI--
defcon.config-file = tests/bool_basic01.conf
--FILE--
<?php
echo "TRUTH: '" . TRUTH . "'\n";
?>
--EXPECT--
TRUTH: '1'
