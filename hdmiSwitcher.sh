#!/bin/bash


FILESTATE=`ls -l /boot/ | grep -o hdmi_screen`
HDMI_STATUS=`tvservice -l | grep -o HDMI`

if [[ $FILESTATE != "hdmi_screen" ]]
then
    sudo cp -f /boot/config_hdmi.txt /boot/config.txt
    sudo touch /boot/hdmi_screen
    sudo rm -f /boot/lcd_screen
    echo "Switching to HDMI"
    #reboot
else
    sudo cp -f /boot/config_lcd.txt /boot/config.txt
    sudo touch /boot/lcd_screen
    sudo rm -f /boot/hdmi_screen
    echo "Switching to LCD Screen"
    #reboot
fi
touch /home/pi/shutmedown.txt
