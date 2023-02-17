#!/bin/ksh
#:#################################:#
#:# Build & publish               #:#
#:#                               #:#
#:# Using ssh keys is recommended #:#
#:#                               #:#
#:# Example:                      #:#
#:# $ ./publish pi@192.168.1.25   #:#
#:#################################:#

usage() {
  echo "./publish.sh user@server"
}

if [ $# -lt 1 ];then
  usage
  exit 2
fi

SERVER=$1
TARGET=$SERVER:bin/

make clean || exit 2
make       || exit 2

ssh $SERVER bin/stop.sh         || exit 2
ssh $SERVER rm -f bin/streetcar || exit 2
scp bin/streetcar $TARGET       || exit 2
ssh $SERVER bin/start.sh        || exit 2
