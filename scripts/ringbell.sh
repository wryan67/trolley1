#!/bin/ksh
set -a
[ "$SUDO_USER" = "" ] && USERID=$LOGNAME || USERID=$SUDO_USER
USERHOME=/home/$USERID

if [ $# -lt 1 ];then
  echo "usage: ringbell.sh [east|west|both]"
  exit 1
fi

shutdown() {
  exit 0
}


trap shutdown 0 1 2 3 9 15

typeset -l DIRECTION
DIRECTION=$1
VOLUME=$2
AUDIODEV="$3"

if [ "$VOLUME" = "" ];then
  VOLUME=0.1
  VOLUME=1.0
fi

if [ "$DIRECTION" = "both" ];then
  MODE=1
else 
  SOUND="/home/$USERID/trolleyBell-${DIRECTION}.mp3"
  MODE=2
fi

if [ ! -f $SOUND ];then
  echo "cannot find sound file:  $SOUND"
  exit 1
fi
#omxplayer -o local $SOUND > /dev/null 2>&1 &


if [ "$AUDODEV" = "" ];then
# AUDIODEV="dmix:CARD=Set,DEV=0"
# AUDIODEV="bluealsa:HCI=hci0,DEV=76:90:46:36:99:4C,PROFILE=a2dp"
  AUDIODEV="dmix:CARD=Device,DEV=0"
fi

#aplay -l
# usage: amixer -C = card
#sudo -E amixer set PCM 10%
#sudo -E amixer -c2 set PCM 100% unmute
#sudo -E amixer -c1 100% 

#echo $AUDIODEV

if [ $MODE = 2 ];then
  nohup play -v $VOLUME $SOUND > /dev/null 2>&1 &
  exit
else
  while [ 1 ];do
    for DIRECTION in east west;do
      echo Ringing $DIRECTION bell...
      SOUND="/home/$USERID/trolleyBell-${DIRECTION}.mp3"
      nohup play -v $VOLUME $SOUND > /dev/null 2>&1 
    done
  done
fi




