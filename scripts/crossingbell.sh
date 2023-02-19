#!/bin/ksh
set -a
[ "$SUDO_USER" = "" ] && USERID=$LOGNAME || USERID=$SUDO_USER
USERHOME=/home/$USERID

VOLUME=$1
DURATION=$2
SOUND="$USERHOME/rr-crossing-bells.48k.wav"

if [ "$VOLUME" = "" ];then
  VOLUME=2.0
fi

if [ "$DURATION" = "" ];then
  DURATION=`soxi -d $SOUND`
fi



#omxplayer -o local $SOUND > /dev/null 2>&1 &


AUDIODEV="$3"

if [ "$AUDODEV" = "" ];then
# AUDIODEV="dmix:CARD=Set,DEV=0"
# AUDIODEV="dmix:CARD=Device,DEV=0"
# AUDIODEV="dmix:CARD=GS3,DEV=0"
# AUDIODEV="dmix:CARD=Device,DEV=0"
# AUDIODEV="bluealsa:HCI=hci0,DEV=76:90:46:36:99:4C,PROFILE=a2dp"
  AUDIODEV="dmix:CARD=Device_1,DEV=0"
fi

#sudo -E amixer set PCM 10%
#sudo -E amixer -c 2 set PCM 100% unmute

#amixer -c1 scontrols 
#amixer -c3 sset Speaker '100%' > /dev/null 2>&1

echo playing...
play -v $VOLUME $SOUND fade "0:0" "$DURATION" "0:1.5" > /dev/null 2>&1
# > /dev/null 2>&1 &
