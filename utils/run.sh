#!/bin/bash

# Author: Johan Grahn
# Date: 2010-05-28

# Configurations 
BINARY=../build/pride 
REPLICAS=2
BUILD_TYPE="debug"
START_PORT=50000

# Tell bash to quit if any commands fails
# set -e

# Chedck if the number of replicas is included 
if [ "$#" -gt "0" ] 
then
	REPLICAS=$1
	BUILD_TYPE=$2
	WRITERS=$3
fi

PORT=$START_PORT
LISTEN_PORT=$START_PORT
WRITER=""

if [ "$BUILD_TYPE" == "debug" ] 
then 
	echo "Building debug binary"
	cmake -DCMAKE_BUILD_TYPE=Debug -DNUM_REPLICAS=$REPLICAS ..
else 
	echo "Building release binary"
	cmake -DCMAKE_BUILD_TYPE=Release -DNUM_REPLICAS=$REPLICAS ..
fi 

make

# Remove all previous pride processes 
pkill pride 

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
	else
		if [ "$WRITERS" == "1" ] 
		then 
			WRITER="-w"
		else
			WRITER=""
		fi
	fi 
	echo $BINARY -i $i $WRITER -l $LISTEN_PORT $HOSTS -f $i.log 
	$BINARY -i $i  $WRITER -l $LISTEN_PORT $HOSTS -f $i.log &
	
	echo "Starting replica nr $i"
	# Increment the listen port
	LISTEN_PORT=$[$LISTEN_PORT + 1]

done 



