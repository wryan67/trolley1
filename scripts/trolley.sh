#!/bin/ksh
set -a
[ "$SUDO_USER" = "" ] && USERID=$LOGNAME || USERID=$SUDO_USER
USERHOME=/home/$USERID

. $USERHOME/bin/env.`hostname -s`.sh

$USERHOME/bin/stop.sh $1


if [ "$DHT_PIN" = "" ];then
  DHT_PIN=23
fi

if [ "$LCD_ADDRESS" = "" ];then
  LCD_ADDRESS=`gpio i2cd | cut -c 4-99 | sed 's/--//g' | awk '{if (NR>1 && NF>0) print $1}'|head -1`
fi

echo LCD Address=$LCD_ADDRESS
MSG="$*"

if [ "$LCD_ADDRESS" = "" ];then
   echo "LCD not detected"
   exit 9
fi

# portram
#TROLLING=150
#MAX_SPEED=500
#EAST_DECTIME=750
#WEST_DECTIME=750

EXE="$USERHOME/bin/streetcar"
LOG="$USERHOME/logs/trolley.log"

mkdir -p $USERHOME/logs

sudo $USERHOME/bin/fixPerms.sh 

date "+%Y%m%d %H:%M:%S" > $LOG
echo "$EXE -d -a $LCD_ADDRESS -t$TROLLING -m$MAX_SPEED -e$EAST_DECTIME -w$WEST_DECTIME $*" >> $LOG
nohup $EXE -d -a $LCD_ADDRESS -t$TROLLING -m$MAX_SPEED -e$EAST_DECTIME -w$WEST_DECTIME $*  >> $LOG 2>&1 &

