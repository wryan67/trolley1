#!/bin/ksh
set -a

. ~/bin/env.`hostname -s`.sh

~/bin/stop.sh $1


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


# holiday trolley
#EAST_DECTIME=500
#WEST_DECTIME=600
#TROLLING=50
#MAX_SPEED=120

# portram
TROLLING=150
MAX_SPEED=500
EAST_DECTIME=750
WEST_DECTIME=750

EXE="~/projects/streetCar/bin/ARM/Debug/streetCar.out"
EXE="/home/wryan/bin/streetcar"
LOG="/home/wryan/logs/trolley.log"

sudo /root/bin/fixPerms.sh > /dev/null 2>&1

#nohup ~/projects/streetCar/bin/ARM/Debug/streetCar.out -d -a $LCD_ADDRESS -t$TROLLING -m$MAX_SPEED -e$EAST_DECTIME -w$WEST_DECTIME $* > ~/logs/trolley.log 2>&1 &

date "+%Y%m%d %H:%M:%S" > $LOG
echo "$EXE -d -a $LCD_ADDRESS -t$TROLLING -m$MAX_SPEED -e$EAST_DECTIME -w$WEST_DECTIME $*" >> $LOG
nohup $EXE -d -a $LCD_ADDRESS -t$TROLLING -m$MAX_SPEED -e$EAST_DECTIME -w$WEST_DECTIME $*  >> $LOG 2>&1 &

