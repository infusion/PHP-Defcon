--TEST--
See that require of missing file fails
--INI--
defcon.config-file = tests/require_error01.conf
--FILE--
<?php
echo DEFCON_GOOD_STRING . "\n";
echo DEFCON_GOOD_BOOL . "\n";
?>
--EXPECTF--
Fatal error: defcon: tests/require_error01.conf.inc.missing line 1: Cannot open for reading in Unknown on line 0
Good String Content

Notice: Use of undefined constant DEFCON_GOOD_BOOL - assumed 'DEFCON_GOOD_BOOL' in %s
DEFCON_GOOD_BOOL
