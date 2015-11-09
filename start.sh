#!/bin/bash
{
    sleep 2; xhost +
} &
nohup Xorg -noreset -logfile /root/0.log -config /root/xorg.conf :0 &
/root/cdp-server -p 5999 -d :0