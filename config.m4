PHP_ARG_ENABLE(defcon, for Defcon support, [  --enable-defcon          Enable Defcon support])

if test "$PHP_DEFCON" != "no"; then
  AC_DEFINE(HAVE_DEFCON, 1, [ ])
  PHP_NEW_EXTENSION(defcon, defcon.c, $ext_shared)
fi

