#!/bin/bash

### BEGIN INIT INFO
# Provides: hdmiSwitcher
# Required-Start: $remote_fs $syslog
# Required-Stop: $remote_fs $syslog
# Default-Start: 2 3 4 5
# Default-Stop: 0 1 6 
### END INIT INFO


case "$1" in 
    start)
	HDMI_STATUS=`tvservice -l | grep -o HDMI`
	if [[ $HDMI_STATUS = "HDMI" ]]
	then
            FILESTATE=`ls -l /boot/ | grep -o hdmi_screen`
	    if [[ $FILESTATE != "hdmi_screen" ]]
	    then
		cp -f /boot/config_hdmi.txt /boot/config.txt
		touch /boot/hdmi_screen
		rm -f /boot/lcd_screen
		echo "HDMI Display available, restarting to have hdmi as primary display"
		reboot
	    fi
	else
            FILESTATE=`ls -l /boot/ | grep -o lcd_screen`
	    if [[ $FILESTATE != "lcd_screen" ]]
	    then
		cp -f /boot/config_lcd.txt /boot/config.txt
		touch /boot/lcd_screen
		rm -f /boot/hdmi_screen
		echo "Internal Display only, restarting with the matching config"
		reboot
	    fi
	fi
     ;;
    *)
    echo "script only active when called with 'start'"
    ;;
esac
