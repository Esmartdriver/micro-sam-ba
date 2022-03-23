#!/bin/bash
echo "ACM Ports Available:"
ls /dev/ |grep ACM
ACM_ARRAY=(`ls /dev/ |grep ACM`)
IDENTIFIER='ID_MODEL_FROM_DATABASE=at91sam SAMBA bootloader'
#udevadm info -q property -n /dev/ttyACM1 | grep -oP '(?<=ID_SERIAL=).*' 
for i in "${ARRAY[@]}"
do
   OUTPUT=$(udevadm info -q property -n /dev/$i)

   if [[ "$OUTPUT" =~ .*"$IDENTIFIER".* ]]; then
      echo "Identified port with Samba bootloader:" $ACM_ARRAY
      ACM=$ACM_ARRAY
       
      ./usamba /dev/$ACM read image.bin 0x00003000 32

   fi  
done
