--TEST--
basic backtick substitution
--INI--
defcon.config-file = tests/backtick_basic01.conf
--FILE--
<?php
echo DEFCON_TICKED_STRING . "\n";
echo DEFCON_TICKED_INT . "\n";
?>
--EXPECT--
hello world
42
