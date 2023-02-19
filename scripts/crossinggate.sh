#!/bin/ksh
set -a
[ "$SUDO_USER" = "" ] && USERID=$LOGNAME || USERID=$SUDO_USER
USERHOME=/home/$USERID

. $USERHOME/env.pitrain.sh

typeset -l DIRECTION=`echo $1 | cut -c1`

up() {
  pca9685 -p${CrossingGate1} -f $CrossingGate1Up > /dev/null 2>&1
  pca9685 -p${CrossingGate2} -f $CrossingGate2Up > /dev/null 2>&1
}

dn() {
  pca9685 -p${CrossingGate1} -f $CrossingGate1Dn > /dev/null 2>&1
  pca9685 -p${CrossingGate2} -f $CrossingGate2Dn > /dev/null 2>&1
}

shutdown() {
  exit 0
}

trap shutdown 0 1 2 3 9 15


if [ "$DIRECTION" = "u" ];then
  up
  exit
fi

if [ "$DIRECTION" = "d" ];then
  dn
  exit
fi

if [ "$DIRECTION" = "l" ];then
  while [ 1 ];do
    dn
    sleep 1
    up
    sleep 1
  done  
  exit
fi

echo 'usage: crossing [up|down|loop]'
