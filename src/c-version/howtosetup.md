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


## Set up X-Mem in babybel3

1. `(emmentaler1):~$ scp -r ./2021-msc-atriantaf-code/ root@babybel3:~`
1. `(babybel3):~# apt install scons`
1. `(babybel3):~# apt install libnuma-dev`
1. `(babybel3):~# apt install libhugetlbfs-dev`
1. `(babybel3):~# cd 2021-msc-atriantaf-code/X-Mem`
1. `(babybel3):~/2021-msc-atriantaf-code/X-Mem# ./build-linux.sh x64 20`


## Build a custom kernel

1. `(babybel3):~# apt update`
1. `(babybel3):~# apt install build-essential`
 git clone git://kernel.ubuntu.com/ubuntu/ubuntu-trusty.git

1. `(babybel3):~/linux-3.13.0# make menuconfig`
    1. Select `Kernel hacking`
    1. Press `n` on `Filter access to /dev/mem`
    1. Exit to main menu
    1. Select `General setup`
    1. Press `y` on `Kernel .config support`
    1. Press `y` on `Enable access to .config through /proc/config.gz`

  410  mkdir build
  411  ls
  412  cp /boot/config-3.13.0-24-generic ./build/.config
  413  make menuconfig O=./build
  414  make O=./build
  415  make O=./build -j 20
  416  ls build/
  417  make modules_install O=./build -j 20
  418  ls build/
  419  ls build/lib/
  420  ls build/lib/modules.builtin
  421  ls
  422  ls init/
  423  find . -name "initrd" 2> /dev/null
  424  find . -name "initrd*" 2> /dev/null
  425  find . -name "*initrd*" 2> /dev/null
  426  make install O=./build -j 20
  427  make install O=./build -j 20 INSTALL_PATH=./build/
  428  make install O=./build -j 20 INSTALL_PATH=./build
  429  pwd
  430  make install O=./build -j 20 INSTALL_PATH=.
  431  make install O=./build -j 20 INSTALL_PATH=./build
  432  pwd
  433  make install O=./build -j 20 INSTALL_PATH=/root/ubuntu-trusty/build
  434  mkdir build/b
  435  make install O=./build -j 20 INSTALL_PATH=/root/ubuntu-trusty/build/b
  436  ls build/b
  437  /usr/sbin/update-initramfs.orig.initramfs-tools
  438  mkinitramfs
  439  mkinitramfs  -d ./build/
  440  mkinitramfs  -o ./build/aa
  441  ls build/
  442  cat aa
  443  cat build/aa
  444  make install O=./build -j 20 INSTALL_PATH=/root/ubuntu-trusty/build/
  445  make install O=./build -j 20 INSTALL_PATH=/root/ubuntu-trusty/build/b
  446  ls build/b
  447  make install O=./build -j 20 INSTALL_PATH=/root/ubuntu-trusty/build/b
  448  apt install grub
  449  make install O=./build -j 20 INSTALL_PATH=/root/ubuntu-trusty/build/b
  450  ls build/b
  451  ls build/
  452  ls /boot/
  453  make install O=./build -j 20 INSTALL_PATH=/root/ubuntu-trusty/build/b
  454  ls
  455  cd build/
  456  ls
  457  whereis update-initramfs
  458  cat /usr/sbin/update-initramfs
  459  #mkinitramfs -o /boot/initrd.img-${kernel_ver}-generic ${kernel_ver}-generic
  460  ls /boot/
  461  mkinitramfs -o /boot/initrd.img-3.13.11-ckt39 3.13.11-ckt39
  462  mkinitramfs
  463  ls
  464  ls source
  465  mkinitramfs -o /boot/initrd.img-3.13.11-ckt39 3.13.11-ckt39
  466  mkinitramfs -d . -o /boot/initrd.img-3.13.11-ckt39 3.13.11-ckt39
  467  find / -name "initramfs.conf"
  468  find / -name "initramfs.conf" 2>/dev/null
  469  cp /etc/initramfs-tools/initramfs.conf .
  470  mkinitramfs -d . -o /boot/initrd.img-3.13.11-ckt39 3.13.11-ckt39
  471  make modules_install
  472  ls /lib/modules
  473  mkinitramfs -o /boot/initrd.img-3.13.11-ckt39 3.13.11-ckt39
  474  mkinitramfs -o /boot/initrd.img-3.13.11-ckt39+ 3.13.11-ckt39+
  475  ls
  476  ls
  477  ls b
  478  ls /boot/



## Set up PHI in babybel3

1. `(babybel3):~# apt update`
1. `(babybel3):~# apt install alien`
1. `(babybel3):~# wget http://registrationcenter-download.intel.com/akdlm/irc_nas/15904/mpss-3.8.6-linux.tar`
1. `(babybel3):~# tar -xvf mpss-3.8.6-linux.tar`
1. `(babybel3):~# cd mpss-3.8.6/`
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

