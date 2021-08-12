# How to do stuff on rackservers

## Set up babybel3

1. `(emmentaler1):~$ export PATH=$PATH:/mnt/netos/tools/bin/`
1. `(emmentaler1):~$ echo $PATH`
1. `(emmentaler1):~$ rackboot.sh -l ubuntu-14.04 babybel3`
1. `(babybel3):~$ sudo passwd -d root`
1. `(babybel3):~$ su root`
1. `(babybel3):~# cd`
1. `(babybel3):~# ssh-keygen`
1. copy from emmentaler1 the content of `~/.ssh/id_rsa.pub` to babybel3's `~/.ssh/authorized_keys`


## Build a custom kernel

1. Load Ubuntu 14 in `babybel3`
1. `(babybel3):~# apt update`
1. `(babybel3):~# apt install build-essential`
1. `(babybel3):~# apt-get source linux-image-unsigned-3.13.0`
1. `(babybel3):~/linux-3.13.0# cp /boot/config-3.13.0-24-generic .config`
1. `(babybel3):~/linux-3.13.0# make menuconfig`
    1. Select `Kernel hacking`
    1. Press `n` on `Filter access to /dev/mem`
    1. Exit to main menu
    1. Select `General setup`
    1. Press `y` on `Kernel .config support`
    1. Press `y` on `Enable access to .config through /proc/config.gz`
1. `(babybel3):~/linux-3.13.0# make`
1. `(emmentaler1):~$ scp -r /mnt/netos/tftpboot/ubuntu-14.04-amd64/initrd-3.13.0+.lz  root@babybel3:~`
1. `(babybel3):~/linux-3.13.0# mkdir lztempdir`
1. `(babybel3):~/linux-3.13.0# cd lztempdir`
1. `(babybel3):~/linux-3.13.0/lztempdir lzma -dc -S .lz /root/initrd-3.13.0+.lz | cpio -imvd --no-absolute-filenames`
1. `(babybel3):~/linux-3.13.0/lztempdir# cd ..`
1. `(babybel3):~/linux-3.13.0# make modules_install INSTALL_MOD_PATH=./lztempdir/ -j 20`
1. `(babybel3):~/linux-3.13.0# cd lztempdir`
1. `(babybel3):~/linux-3.13.0# find . | cpio --quiet --dereference -o -H newc | lzma -7 > ../initrd.lz`


## Set up X-Mem in babybel3

1. `(emmentaler1):~$ scp -r ./2021-msc-atriantaf-code/ root@babybel3:~`
1. `(babybel3):~# apt install scons`
1. `(babybel3):~# apt install libnuma-dev`
1. `(babybel3):~# apt install libhugetlbfs-dev`
1. `(babybel3):~# cd 2021-msc-atriantaf-code/X-Mem`
1. `(babybel3):~/2021-msc-atriantaf-code/X-Mem# ./build-linux.sh x64 20`


## Set up PHI in babybel3

1. `(babybel3):~# apt update`
1. `(babybel3):~# apt install alien`
1. `(babybel3):~# wget http://registrationcenter-download.intel.com/akdlm/irc_nas/15904/mpss-3.8.6-linux.tar`
1. `(babybel3):~# tar -xvf mpss-3.8.6-linux.tar`
1. `(babybel3):~# cd mpss-3.8.6`
1. `(babybel3):~/mpss-3.8.6# alien --scripts *.rpm`
1. `(babybel3):~/mpss-3.8.6# dpkg -i *.deb`
1. `(babybel3):~/mpss-3.8.6# echo '/usr/lib64' >> /etc/ld.so.conf.d/zz_x86_64-compat.conf`
1. `(babybel3):~/mpss-3.8.6# ldconfig`

1. `(babybel3):~/mpss-3.8.6# cd`
1. `(babybel3):~# git clone https://github.com/pentschev/mpss-modules.git`
1. `(babybel3):~# cd mpss-modules`
1. `(babybel3):~/mpss-modules# make`
1. `(babybel3):~/mpss-modules# make install`
1. `(babybel3):~/mpss-modules# depmod`
1. `(babybel3):~/mpss-modules# rmmod mic_host`
1. `(babybel3):~/mpss-modules# modprobe mic`
1. `(babybel3):~/mpss-modules# cd`
1. `(babybel3):~# ssh-keygen`
1. `(babybel3):~# micctrl --initdefaults`
1. `(babybel3):~# cp /etc/mpss/mpss.ubuntu /etc/init.d/mpss`
1. `(babybel3):~# update-rc.d mpss defaults 99 10`
1. `(babybel3):~# service mpss start`
1. `(babybel3):~# ssh mic0`


## Run X-Mem on PHI

1. `(babybel3):~# apt install scons`
1. `(babybel3):~# echo "ExtraCommandLine nopat" >> /etc/mpss/mic0.conf`
1. `(babybel3):~# micctrl -r`
1. `(babybel3):~# micctrl -b`
1. `(babybel3):~# cd X-Mem`
1. replace `gcc` w/ `/usr/linux-k1om-4.7/bin/x86_64-k1om-linux-gcc` in `SConscript`.
1. `(babybel3):~/X-Mem# ./build-linux.sh gcc_mic 20`
1. `(babybel3):~/X-Mem# scp -r ./bin/xmem-linux-gcc_mic mic0:~`
1. `(babybel3):~/X-Mem# cd`
1. `(babybel3):~# ssh mic0`



root@ubuntu:~# sudo tar czf ./sdb2/backup.tar.gz --exclude=/backup.tar.gz --exclude=/dev --exclude=/mnt-exclude=/proc --exclude=/sys --exclude=/tmp --exclude=/lost+found --exclude=/root --exclude=/cdrom /

