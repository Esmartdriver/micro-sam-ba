#!/bin/bash
echo "ACM Ports Available:"
ls /dev/ |grep ACM
ARRAY=(`ls /dev/ |grep ACM`)
IDENTIFIER='ID_MODEL_FROM_DATABASE=at91sam SAMBA bootloader'
for i in "${ARRAY[@]}"
do
   OUTPUT=$(udevadm info -q property -n /dev/$i)
   if [[ "$OUTPUT" =~ .*"$IDENTIFIER".* ]]; then
      echo "Identified port with Samba bootloader:" $ARRAY
      ACM=$ARRAY
   fi  
done


EXITCONDITION=false

while (!($EXITCONDITION));
do
echo "Enter the following letter to specify which action you would like to do:"
echo "W - Write"
echo "R - Read"
echo "E - Erase"
echo "X - Exit Script"
echo  -n "Enter your Selection:"
read answer
echo "You've selected "$answer
if [ $answer == 'W' ]
then
	echo  -n "Enter Address to write at:"
    read address
    ./usamba /dev/$ACM write image.bin $address 
elif [ $answer == 'R' ]
then
	echo  -n "Enter Address to read at:"
    read address
    ./usamba /dev/$ACM read image.bin $address 32
elif [ $answer == 'E' ]
then
	echo Erasing...; ./usamba /dev/$ACM erase-all 
elif [ $answer == 'X' ]
then
	echo exiting program;EXITCONDITION=true
else 
	echo Invalid Command 
fi 
done
#./usamba /dev/$ACM read image.bin 0x00003000 32
