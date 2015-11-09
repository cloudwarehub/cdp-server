#!/bin/bash
xhost +
nohup Xorg -noreset -logfile /root/0.log -config /root/xorg.conf :0 &
sleep 2;
/root/cdp-server -p 5999 -d :0