--TEST--
double quoted string nul characters
--INI--
defcon.config-file = tests/quoted_double_basic06.conf
--FILE--
<?php
echo DEFCON_STRING_1 . "\n";
echo DEFCON_STRING_2 . "\n";
?>
--EXPECTF--
Fatal error: defcon: tests/quoted_double_basic06.conf line 2: no '\0' allowed in Unknown on line 0

Notice: Use of undefined constant DEFCON_STRING_1 - assumed 'DEFCON_STRING_1' in %s
DEFCON_STRING_1

Notice: Use of undefined constant DEFCON_STRING_2 - assumed 'DEFCON_STRING_2' in /%s
DEFCON_STRING_2
