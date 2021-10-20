#!/bin/bash

mode=`cat /opt/retropie/startup.config`  
echo $mode
if [[ $mode = "DESKTOP" ]]
 then 
    echo "starting desktop mode"
    startx
fi
if [[ $mode = "EMULATIONSTATION" ]] 
then
    echo "starting emulationstation"
    emulationstation
fi
#emulationstation #auto
