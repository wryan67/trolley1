#!/bin/ksh
set -a
[ "$SUDO_USER" = "" ] && USERID=$LOGNAME || USERID=$SUDO_USER
USERHOME=/home/$USERID

. $USERHOME/bin/env.`hostname -s`.sh

releaseLock() {
  rm -rf /tmp/start.lock
}

shutdown() {
  releaseLock
  exit 0
}


trap shutdown 0 1 2 3 9 15

[ ! -f $USERHOME/pitrain.i2cd ] && gpio i2cd > $USERHOME/pitrain.i2cd
I2CD_MASTER=`cat $USERHOME/pitrain.i2cd | awk '{print $1}'`
I2CD_CURR=`gpio i2cd | awk '{print $1}'`

if [ "$I2CD_CURR" != "$I2CD_MASTER" ];then echo
  echo "i2cd footprint doesn't match the master"
  exit 2
fi

mkdir /tmp/start.lock > /dev/null 2>&1
RET=$?

if [ $RET = 0 ];then
  CT=`ps -ef | grep -i "streetCar" | grep -v grep | wc -l`

  if [ $CT -lt 1 ];then
    $USERHOME/bin/trolley.sh
  fi
else
  echo "start is locked"
fi

