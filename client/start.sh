#!/bin/bash

LOGFILE=./logs/dbclient.`date +%d%m%y%H%M%S`
TWSIP=10.0.1.2

PID=`ps -ef | grep dbclient | grep -v grep | awk '{ print $2 }'`

if [ ! -z $PID ]; then
	echo "Process dbclient pid=$PID is already running"
	exit 0
fi

nohup ./dbclient ${TWSIP} > ${LOGFILE} 2>&1 < /dev/null &

if [ $? == 0 ]; then
	echo ""
	PID=`ps -ef | grep dbclient | grep -v grep | awk '{ print $2 }'`
	echo "Application is started (pid=$PID). Check log file `pwd`/$LOGFILE"
	echo "  tail -f `pwd`/$LOGFILE"
	echo ""
else
	echo " --- ERROR ---"
	echo "Application failed to start. Check ini and log file `pwd`/($LOGFILE)"
	echo " -------------"
fi
