#!/bin/sh

echo "intltoolize..."
intltoolize -c --force || exit 1

echo "aclocal..."
aclocal || exit 1

echo "autoheader..."
autoheader || exit 1

echo "autoconf..."
autoconf || exit 1

echo "libtoolize..."
libtoolize || exit 1

echo "automake..."
automake -a -c || exit 1
