#!/bin/bash

# Make sure to call this as "./config_ipbus.sh", not "source config_ipbus.sh"

echo "Power Cycling"
#ssh piRBDev "sudo /home/pi/RDOUT_BOARD_IPBus/bin/pwr_cycle_fpgas.exe" &
ssh pi2 "sudo /home/pi/RDOUT_BOARD_IPBus/bin/pwr_cycle_fpgas.exe" &
ssh pi3 "sudo /home/pi/RDOUT_BOARD_IPBus/bin/pwr_cycle_fpgas.exe" &
ssh pi4 "sudo /home/pi/RDOUT_BOARD_IPBus/bin/pwr_cycle_fpgas.exe" &
ssh pi5 "sudo /home/pi/RDOUT_BOARD_IPBus/bin/pwr_cycle_fpgas.exe" &
wait # wait for all the background jobs

#echo "Power Cycling Done -> wait 10 seconds"
#sleep 10

echo "Setting IP bus IP"
#ssh piRBDev "cd /home/pi/RDOUT_BOARD_IPBus/rdout_software/ ;/home/pi/RDOUT_BOARD_IPBus/rdout_software/ip_mac_addr.sh" &
ssh pi2 "cd /home/pi/RDOUT_BOARD_IPBus/rdout_software/ ;/home/pi/RDOUT_BOARD_IPBus/rdout_software/ip_mac_addr.sh" &
ssh pi3 "cd /home/pi/RDOUT_BOARD_IPBus/rdout_software/ ;/home/pi/RDOUT_BOARD_IPBus/rdout_software/ip_mac_addr.sh" & 
ssh pi4 "cd /home/pi/RDOUT_BOARD_IPBus/rdout_software/ ;/home/pi/RDOUT_BOARD_IPBus/rdout_software/ip_mac_addr.sh" &
ssh pi5 "cd /home/pi/RDOUT_BOARD_IPBus/rdout_software/ ;/home/pi/RDOUT_BOARD_IPBus/rdout_software/ip_mac_addr.sh" &
wait # wait for all the background jobs

echo "Setting IP bus Done -> wait for ping"
PING=`which ping`
function waitForHost
{
    reachable=0;
    while [ $reachable -eq 0 ];
    do
	$PING -q -c 1 $1
	if [ "$?" -eq 0 ];
	then
            reachable=1
	    sleep 1
	fi
    done
}

#waitForHost 192.168.222.200 &
waitForHost 192.168.222.201 &
waitForHost 192.168.222.202 &
waitForHost 192.168.222.203 &
waitForHost 192.168.222.204 &
wait # wait for all the background jobs

echo "Ping Done"


