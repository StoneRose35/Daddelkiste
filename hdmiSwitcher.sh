#!/bin/bash

### BEGIN INIT INFO
# Provides: hdmiSwitcher
# Required-Start: $remote_fs $syslog
# Required-Stop: $remote_fs $syslog
# Default-Start: 2 3 4 5
# Default-Stop: 0 1 6 
# END INIT INFO

HDMI_STATUS=`tvservice -l | grep -o HDMI`
if [[ $HDMI_STATUS = "HDMI" ]]
then
    if [[ `ls -l /boot/ | grep -o hdmi_screen` != "hdmi_screen" ]]
    then
        cp -f /boot/config_hdmi.txt /boot/config.txt
        touch /boot/hdmi_screen
        rm -f /boot/lcd_screen
        echo "HDMI Display available, restarting to have hdmi as primary display"
        reboot
    fi
else
    if [[ `ls -l /boot/ | grep -o ldc_screen` != "lcd_screen" ]]
    then
        cp -f /boot/config_lcd.txt /boot/config.txt
        touch /boot/lcd_screen
        rm -f /boot/hdmi_screen
        echo "Internal Display only, restarting with the matching config"
        reboot
    fi
fi
