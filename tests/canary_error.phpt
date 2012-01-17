--TEST--
Test for "canary mismatch on efree" with PHP 5.2 / 32bit
--DESCRIPTION--
Running on PHP 5.2 / 32 bit, I see the following error with defcon.so
	ALERT - canary mismatch on efree() - heap overflow detected
at the end of any script run. This trivial test serves to show that.
--FILE--
<?php
echo "OK\n";
?>
--EXPECT--
OK
