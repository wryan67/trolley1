#!/bin/ksh

[ "$SUDO_USER" = "" ] && USERID=$LOGNAME || USERID=$SUDO_USER
USERHOME=/home/$USERID

chown root.$USERID $USERHOME/bin/streetcar
chmod u+s          $USERHOME/bin/streetcar

