#!/bin/bash

if [[ $EUID -ne 0 ]]; then
   echo "Please run this script as root or under sudo." 
   exit 1
fi

echo core >/proc/sys/kernel/core_pattern; cd /sys/devices/system/cpu && echo performance | tee cpu*/cpufreq/scaling_governor
