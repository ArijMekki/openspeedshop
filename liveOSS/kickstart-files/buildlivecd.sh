#!/bin/sh

livecd_label=OpenSpeedShop-Live
livecd_kickstart_file=livecd-fedora-15-openss.ks

#You must run livecd-creator as root...
if [ `whoami` != "root" ]; then
    echo "remember: you must be root to run livecd-creator..."
fi

# su -c "livecd-creator -d -v --config=$livecd_kickstart_file \
su -c "livecd-creator --config=$livecd_kickstart_file --fslabel=$livecd_label" root
