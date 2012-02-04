--TEST--
backtick substitution in a concat sequence
--INI--
defcon.config-file = tests/backtick_basic02.conf
--FILE--
<?php
echo DEFCON_PREFIXED_TICK . "\n";
echo DEFCON_EMBEDDED_TICK . "\n";
?>
--EXPECT--
They say: hello world
I think that is cool!
