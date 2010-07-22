#!/bin/sh
aclocal
autoheader
automake --add-missing --gnu
autoconf
