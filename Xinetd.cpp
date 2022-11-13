
// 查看是否安装xinetd: sudo /etc/init.d/xinetd status
// 查看xinetd的PID： cat /run/xinetd.pid
// 安装xinetd： sudo apt-get install xinetd
// 出现The unit file, source configuration file or drop-ins of xinetd.service changed on disk，
// 运行sudo systemctl daemon-reload进行配置重载。
// 不要安装telnet: sudo apt-get install telnet, 会删除xinetd服务。
// 重启xinetd进程：sudo service xinetd restart
// 在/etc/xinetd.d/下存在各种服务，将/etc/xinetd.d/telnet中的disable值由yes改为no，重启即可。
// 如果Server /usr/sbin/in.telnetd is not executable，则需要安装telnet服务器：sudo apt-get install telnetd
// 启动telnet服务器： /usr/sbin/in.telnetd start