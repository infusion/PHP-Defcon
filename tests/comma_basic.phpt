--TEST--
comma_basic.conf configuration reading
--INI--
defcon.config-file = tests/comma_basic.conf
--FILE--
<?php
echo DEFCON_GOOD_STRING . "\n";
echo DEFCON_SECOND_STRING . "\n";
echo DEFCON_ANOTHER_STRING . "\n";
?>
--EXPECT--
Good String Content
/works/when/unquoted
Same Keyword, Same Type
