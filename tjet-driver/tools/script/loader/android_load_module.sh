#!/system/bin/sh

#
# setup paramters
#
case "$1" in

"FIT" | "Fit" | "fit")
module_list="sdio sdiocore tososcmn tosbuscmn toscnlfit tosiofit toscnl"
device_name[0]="CnlFitCtrl"
device_name[1]="CnlFitAdpt"
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
    exec 3<&-
    para_1=${para_1%,}
    para_2=${para_2%,}
fi


#
# install module
#
echo "### Install driver modules ..."

for module_name in $module_list
do
    echo -n "+++ install $module_name ... "
    if [ "$module_name" == "tososcmn" ]; then
        insmod "$obj_dir/$module_name$module_sfx" "$para_1 $para_2" > $log
    else
        insmod "$obj_dir/$module_name$module_sfx" > $log
    fi
    if [ $? == 0 ]; then
        echo "OK."
    else
        echo "Failed."
    fi
done

#
# make device node
#
echo "### Link device node ..."

for dev_file in ${device_name[*]}
do
    target=/dev/${dev_file}0
    symlink_name=/dev/${dev_file}
    if ! test ${target} -ef ${symlink_name}; then
        rm ${symlink_name} 2> /dev/null
        echo "+++ ${symlink_name} is linked to ${target}"
        ln -s ${target} ${symlink_name}
        chmod 0666 ${symlink_name}
    else
        echo "    ${symlink_name} was linked to ${target}"
    fi
done
