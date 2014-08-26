#!/bin/sh

#
# setup paramters
#
case "$1" in

"FIT" | "Fit" | "fit")
module_list="sdio sdiocore tososcmn tosbuscmn toscnlfit tosiofit toscnl"
device_name0="CnlFitCtrl"
device_name1="CnlFitAdpt"
;;

*)
echo "Usage: load_module.sh [fit] [object-path]"
exit 0
;;
esac


module_sfx=".ko"
device_attr="666"
device_dir="/dev"

log="/dev/null"

if [ $# -eq 2 ] ; then
obj_dir=$2
else
echo "Set default path."
obj_dir=/system/lib/modules
fi
#
# check directory
#
if ! [ -f /proc/modules -a -f /proc/devices ]; then
    echo "\"/proc/modules\" or \"/proc/devices\" file is not found."
    exit 1
fi

if ! [ $obj_dir -a -d $obj_dir ]; then
    echo "wrong directory path!!! please set correct path."
    echo "how to use \"./load_module OBJ_DIR_PATH\""
    exit 1
fi

#
# check RF setting file
#
para_file="rfcnf.ini"
para_file=$obj_dir/$para_file
para_1=
para_2=
if [ -f $para_file ]; then
    regaddr=
    value=
    para_1="RFADRS="
    para_2="RFVAL="
    exec 3< $para_file
    while read regaddr value commnt 0<&3
    do
        para_1=$para_1$regaddr,
        para_2=$para_2$value,
    done
    para_1=`echo $para_1 | sed -e "s/.\$//"`
    para_2=`echo $para_2 | sed -e "s/.\$//"`
    exec 3<&-
fi


#
# install module
#
echo "### Install driver modules ..."

for module_name in $module_list
do
    flag=`cat /proc/modules | grep "$module_name "`
echo ${flag}
    if [ ${#flag[*]} = 0 ]; then
        echo -n "+++ install $module_name ... "
        if [ "$module_name" = "tososcmn" ]; then
            /sbin/insmod "$obj_dir/$module_name$module_sfx" "$para_1" "$para_2" > $log
        else
            /sbin/insmod "$obj_dir/$module_name$module_sfx" > $log
        fi
        if [ $? = 0 ]; then
            echo "OK."
        else
            echo "Failed."
            exit 1
        fi
    else
        echo "$module_name module has been already installed..."
    fi
done

#
# Link device node.
#
echo "### Link device node ..."

    dev_node=`ls -l ${device_dir} | grep $device_name0`
    dev_info=`cat /proc/devices | grep $device_name0`
    if [ ${#dev_info[*]} = 0 ]; then
        echo "$module_name module had not been installed yet or device node unnecessary."
    else
        echo -n "+++ making ${device_name0} device node $device_dir/${device_name0}"
        ln -s $device_dir/${device_name0}0 $device_dir/${device_name0}
        echo "+++ $device_dir/$device_name0 is linked to $device_dir/${device_name0}0"

        chmod $device_attr $device_dir/$device_name0
    fi

    dev_node=`ls -l ${device_dir} | grep $device_name1`
    dev_info=`cat /proc/devices | grep $device_name1`
    if [ ${#dev_info[*]} = 0 ]; then
        echo "$module_name module had not been installed yet or device node unnecessary."
    else
        echo -n "+++ making ${device_name1} device node $device_dir/${device_name1}"
        ln -s $device_dir/${device_name1}0 $device_dir/${device_name1}
        echo "+++ $device_dir/$device_name1 is linked to $device_dir/${device_name1}0"

        chmod $device_attr $device_dir/$device_name1
    fi

