#!/bin/ksh
set -a
[ "$SUDO_USER" = "" ] && USERID=$LOGNAME || USERID=$SUDO_USER
USERHOME=/home/$USERID

. $USERHOME/env.pitrain.sh

DURATION=$1
PCA9635_ADDR="1f"
LED1=`echo $CROSSING_LED1 | awk -F_ '{print $NF}'`
LED2=`echo $CROSSING_LED2 | awk -F_ '{print $NF}'`

MAX=255
OFF=0

stopSignal() {
  pca9635 -a $PCA9635_ADDR -p$LED1 -b 0 > /dev/null 2>&1
  pca9635 -a $PCA9635_ADDR -p$LED2 -b 0 > /dev/null 2>&1
  exit
}


trap stopSignal 0 1 2 3 9 15 

#crossingbell.sh 1 $DURATION > /dev/null 2>&1 &

if [ "$DURATION" = "" ];then
  echo usage: `basename $0` seconds
  exit 9
fi


i=0
while [ $i -lt $DURATION ];do
  pca9635 -a $PCA9635_ADDR -p$LED1 -b $MAX > /dev/null 2>&1
  pca9635 -a $PCA9635_ADDR -p$LED2 -b $OFF > /dev/null 2>&1
  sleep 0.5

  pca9635 -a $PCA9635_ADDR -p$LED1 -b $OFF > /dev/null 2>&1
  pca9635 -a $PCA9635_ADDR -p$LED2 -b $MAX > /dev/null 2>&1
  sleep 0.5

  ((++i))
done
