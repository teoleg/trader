#!/bin/bash

killall dbclient
if [ $? != 0 ]; then
	printf "\n\t!!ERROR Failed to stop application. kill it manually !! ERROR !!\n\n"
else
	printf "\n\t!!!!! Application has been terminated !!!!!\n\n"
fi
