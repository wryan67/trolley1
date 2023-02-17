#!/bin/ksh

[ "$SUDO_USER" = "" ] && USERID=$USER || USERID=$SUDO_USER
USERHOME=/home/$USERID

chown root.$USERID $USERHOME/bin/streetcar
chmod u+s          $USERHOME/bin/streetcar

