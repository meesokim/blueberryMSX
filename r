#!/bin/bash
cd "$(dirname "$0")"
FILE1="./msx"
FILE2="./menu"
ITEM=`cat ./item`
MSXBUS1='/romtype1 msxbus'
RASMODEL='/proc/device-tree/model'
#amixer cset numid=1 100 > /dev/null
num=1
echo "#!/bin/bash"> msx
chmod +x msx
if [ -f $RASMODEL ]; then
    rasppi=`awk '{ print $3 }' < $RASMODEL`
fi
if [ -z $1 ] && [ "${rasppi}" == "5" ]; then
   echo -n "whiptail --title \"ZemmixBus ${rasppi} - Raspberry Pi MSX Clone\" --menu \"Choose a Machine\" 25 78 16 " >> msx
else
   echo -n "whiptail --title \"ZemmixDrive - Raspberry Pi MSX\" --menu \"Choose a Machine\" 25 78 17 " >> msx
fi
echo -n "" > menu
while machine='' read -r line || [[ -n "$line" ]]; do
	if [[ $line =~ .*$2.* ]]; then
		echo $line >> menu
		echo -n "\"$num\" \"$line\" " >> msx
		num=$((num + 1))
	fi
done < msxmachines

echo -n "--default-item \"\$1\" --clear" >> msx
echo "3>&2 2>&1 1>&3" >> msx 
while true;
do
ITEM=`cat ./item`
choice=$(./msx $ITEM)
if [ -z $choice ]; then 
	tput cvvis
	exit
fi
if [ -z $1 ] && [ "${rasppi}" != "5" ]; then
    MSXBUS=zmxbus
else
    MSXBUS=zmxdrive
    export SDCARD=`lsblk -l | grep vfat | awk '{print $7 }' | sed -n 1p`
    if [ -z $SDCARD ]; then
        unset SDCARD
    fi
fi
a=`sed -n ${choice}p < menu`
echo $choice > ./item
./bluemsx-pi /machine ${a} /romtype1 $MSXBUS /romtype2 $MSXBUS > /dev/null 2>&1 
done
