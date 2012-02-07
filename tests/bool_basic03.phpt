--TEST--
bool value handling
--INI--
defcon.config-file = tests/bool_basic03.conf
--FILE--
<?php
echo "TRUTH: '" . TRUTH . "'\n";
echo "is TRUTH      == true?  " . ((TRUTH      == true) ? "yes" : "NO") . "\n";
?>
--EXPECT--
TRUTH: '1'
is TRUTH      == true?  yes
