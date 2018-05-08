#!/bin/bash

(ssh -T piS "sudo killall sync_debug.exe"; ssh -T piS "nohup sudo /home/pi/SYNCH_BOARD/bin/sync_debug.exe 0 > log.log 2>&1 &"; ssh -T piS "ps -xfa | grep .exe") &
#(ssh -T pi2 "sudo killall new_rdout.exe"; ssh -T pi2 "nohup sudo /home/pi/RDOUT_BOARD_IPBus/rdout_software/bin/new_rdout.exe 200 2000000 0 > log.log 2>&1 &"; ssh -T pi2 "ps -xfa | grep .exe" )&
(ssh -T pi3 "sudo killall new_rdout.exe"; ssh -T pi3 "nohup sudo /home/pi/RDOUT_BOARD_IPBus/rdout_software/bin/new_rdout.exe 200 2000000 0 > log.log 2>&1 &"; ssh -T pi3 "ps -xfa | grep .exe" )&

wait
