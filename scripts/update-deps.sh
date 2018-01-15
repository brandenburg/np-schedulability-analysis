#!/bin/sh


# Expects to be run from main repo dir...

DOCTEST_URL=https://raw.githubusercontent.com/onqtam/doctest/master/doctest/doctest.h

cd lib/include && rm -vf doctest.h && wget $DOCTEST_URL

