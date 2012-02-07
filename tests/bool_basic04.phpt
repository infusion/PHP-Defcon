--TEST--
bool value handling
--INI--
defcon.config-file = tests/bool_basic04.conf
--FILE--
<?php
echo "TRUTH: '" . TRUTH . "'\n";
echo "FALSEHOOD: '" . FALSEHOOD . "'\n";
echo "is TRUTH      == true?  " . ((TRUTH      == true) ? "yes" : "NO") . "\n";
echo "is TRUTH     === true?  " . ((TRUTH     === true) ? "yes" : "NO") . "\n";
echo "is FALSEHOOD  == false? " . ((FALSEHOOD  == false) ? "yes" : "NO") . "\n";
echo "is FALSEHOOD === false? " . ((FALSEHOOD === false) ? "yes" : "NO") . "\n";
echo "is TRUE       == true?  " . ((TRUE       == true) ? "yes" : "NO") . "\n";
echo "is TRUE      === true?  " . ((TRUE      === true) ? "yes" : "NO") . "\n";
echo "is FALSE      == false? " . ((FALSE      == false) ? "yes" : "NO") . "\n";
echo "is FALSE     === false? " . ((FALSE     === false) ? "yes" : "NO") . "\n";
echo "is false      == true?  " . ((false      == true) ? "yes" : "NO") . "\n";
echo "is false     === true?  " . ((false     === true) ? "yes" : "NO") . "\n";
echo "is true       == false? " . ((true       == false) ? "yes" : "NO") . "\n";
echo "is true      === false? " . ((true      === false) ? "yes" : "NO") . "\n";
?>
--EXPECT--
TRUTH: '1'
FALSEHOOD: ''
is TRUTH      == true?  yes
is TRUTH     === true?  yes
is FALSEHOOD  == false? yes
is FALSEHOOD === false? yes
is TRUE       == true?  yes
is TRUE      === true?  yes
is FALSE      == false? yes
is FALSE     === false? yes
is false      == true?  NO
is false     === true?  NO
is true       == false? NO
is true      === false? NO
