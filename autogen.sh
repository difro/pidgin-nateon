#!/bin/sh

echo "aclocal..."
aclocal || exit 1

echo "autoheader..."
autoheader || exit 1

echo "automake..."
automake -a -c || exit 1

echo "autoconf..."
autoconf || exit 1
