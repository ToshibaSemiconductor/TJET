#!/bin/sh

#
# set parameters
#
case "$1" in

"FIT" | "Fit" | "fit")
module_list="toscnl tosiofit toscnlfit tosbuscmn tososcmn sdiocore sdio"
device_name[0]="CnlFitCtrl"
device_name[1]="CnlFitAdpt"
;;

*)
echo "Usage: unload_module.sh [fit]"
exit 0
;;
esac


log="/dev/null"


device_dir="/dev"


#
# check directory
#
if ! [ -d /dev ]; then
    echo "\"/dev\" directry is not found."
    exit 1
fi


#
# remove device node.
#
echo "### Remove device node ..."

i=0

while [ ${device_name[i]} ]
do
    echo -n "--- remove device node $device_dir/${device_name[i]}*..."
    rm $device_dir/${device_name[i]}? > $log
    rm $device_dir/${device_name[i]}  > $log
    if [ $? == 0 ]; then
        echo "OK"
    else
        echo "Failed"
        # pass through
    fi
i=$i+1
done


#
# uninstall module
#
echo "### Uninstall driver modules ..."

for module_name in $module_list
do
    echo -n "--- uninstall $module_name ..."
    /sbin/rmmod $module_name > $log
    if [ $? == 0 ]; then
        echo "OK."
    else
        echo "Failed."
    fi
done
