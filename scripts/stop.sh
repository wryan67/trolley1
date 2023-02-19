#!/bin/ksh
# stop.sh
[ "$SUDO_USER" = "" ] && USERID=$LOGNAME || USERID=$SUDO_USER
USERHOME=/home/$USERID

. $USERHOME/bin/env.pitrain.sh


if [ "$1" = "-watch" ];then
  shift
  KILL="play|[Ss]treetCar|lcdClock|thermostat|crossing" 
else
  KILL="play|[Ss]treetCar|lcdClock|thermostat|crossing|watchTrain" 
fi

CT=`ps -ef | egrep -i "$KILL" | grep -v grep |wc -l`

if [ $CT -gt 0 ];then
  sudo kill -9 `ps -ef | egrep -i "$KILL" | grep -v grep | awk '{print $2}'`
fi

# direction & speed
(
pca9635 -xa $PCA9635_ADDESS -p $SPIN -b 0 
pca9635 -a  $PCA9635_ADDESS -p $WPIN -b 0 
pca9635 -a  $PCA9635_ADDESS -p $EPIN -b 0 
pca9635 -a  $PCA9635_ADDESS -p ${CrossingLED1} -b 0 
pca9635 -a  $PCA9635_ADDESS -p ${CrossingLED2} -b 0 

pca9685 -p${CrossingGate1} -f $CrossingGate1Up 
pca9685 -p${CrossingGate2} -f $CrossingGate2Up 
pca9685 -p${CrossingGate1} -f $CrossingGate1Dn 
pca9685 -p${CrossingGate2} -f $CrossingGate2Dn 
) > /dev/null 2>&1
