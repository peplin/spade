#!/bin/zsh
sudo sysctl -w fs.file-max=128000 
sudo sysctl -w net.ipv4.tcp_keepalive_time=300
sudo sysctl -w net.core.somaxconn=250000
sudo sysctl -w net.ipv4.tcp_max_syn_backlog=2500 
sudo sysctl -w net.core.netdev_max_backlog=2500
