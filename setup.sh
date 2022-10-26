#!/bin/sh
echo "配置主机的共享内存"
sudo sysctl -w kernel.shmmni=28672
sudo sysctl -w kernel.shmmax=134217728
echo "配置后"
cat /proc/sys/kernel/shmmni
cat /proc/sys/kernel/shmmax
echo "开始编译"
make

make install


