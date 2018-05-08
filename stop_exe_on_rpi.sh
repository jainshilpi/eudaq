#!/bin/bash

#ssh -T pi2 "sudo killall new_rdout.exe" &
ssh -T pi3 "sudo killall new_rdout.exe"
ssh -T piS "sudo killall sync_debug.exe" &

wait
