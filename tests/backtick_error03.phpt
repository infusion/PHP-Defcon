--TEST--
backtick substitution overly long value truncation
--INI--
defcon.config-file = tests/backtick_error03.conf
error_log = /dev/null
--FILE--
<?php
echo DEFCON_LONG_STRING . "\n";
?>
--EXPECTF--
Fatal error: defcon: tests/backtick_error03.conf line 21: Value too long in Unknown on line 0

Notice: Use of undefined constant DEFCON_LONG_STRING - assumed 'DEFCON_LONG_STRING' in %s
DEFCON_LONG_STRING
