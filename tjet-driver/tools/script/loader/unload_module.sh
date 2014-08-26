#!/bin/sh

#
# set parameters
#
case "$1" in

"FIT" | "Fit" | "fit")
module_list="toscnl tosiofit toscnlfit tosbuscmn tososcmn sdiocore sdio"
device_name0="CnlFitCtrl"
device_name1="CnlFitAdpt"
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

    echo -n "--- remove device node $device_dir/${device_name0}*..."
    rm $device_dir/${device_name0}  > $log
    if [ $? = 0 ]; then
        echo "OK"
    else
        echo "Failed"
        # pass through
    fi

    echo -n "--- remove device node $device_dir/${device_name1}*..."
    rm $device_dir/${device_name1}  > $log
    if [ $? = 0 ]; then
        echo "OK"
    else
        echo "Failed"
        # pass through
    fi


#
# uninstall module
#
echo "### Uninstall driver modules ..."

for module_name in $module_list
do
    echo -n "--- uninstall $module_name ..."
    /sbin/rmmod $module_name > $log
    if [ $? = 0 ]; then
        echo "OK."
    else
        echo "Failed."
    fi
done
