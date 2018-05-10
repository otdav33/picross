#/bin/sh

git clone https://github.com/otdav33/cwebserv.git
cp cwebserv/cwebserv.{c,h} .
gcc $* -o picross *.c picross.h cwebserv.h
