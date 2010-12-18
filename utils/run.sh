#!/bin/bash

# Author: Johan Grahn
# Date: 2010-05-28

# Configurations 
BINARY=../build/pride 
REPLICAS=2
START_PORT=50000

# Chedck if the number of replicas is included 
if [ "$#" -gt "0" ] 
then
	REPLICAS=$1
fi

PORT=$START_PORT
LISTEN_PORT=$START_PORT
WRITER=""

echo "Creating $REPLICAS replicas"

for (( i=0; i < $REPLICAS; i++ )) 
do
	HOSTS=""
	
	for (( j = 0; j < $REPLICAS; j++ ))
	do
		# Check if we are not adding ourself 
		if [ "$j" != "$i" ] 
		then
			HOSTS="$HOSTS -r $j:127.0.0.1:$[$PORT + $j] "
		fi
	done
	
	
	if [ "$i" == "$[$REPLICAS - 1]" ] 
	then
		WRITER="-w"
	fi 
	
	$BINARY -i $i $WRITER -l $LISTEN_PORT $HOSTS -f $i.log &
	
	echo "Starting replica nr $i"
	# Increment the listen port
	LISTEN_PORT=$[$LISTEN_PORT + 1]

done 



