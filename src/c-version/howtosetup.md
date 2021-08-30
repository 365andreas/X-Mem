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
1. `(babybel3):~# apt install -y build-essential`
1. `(babybel3):~# apt-get source linux-image-unsigned-3.13.0-170-generic`
1. `(babybel3):~# cd linux-3.13.0`
1. `(babybel3):~/linux-3.13.0# scp -r /mnt/local/nfs/ubuntu-14.04-amd64/binary/casper/mount/squashfs-root/boot/config-3.13.0 root@babybel3:/root/linux-3.13.0/.config`
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
1. `(babybel3):~/linux-3.13.0/lztempdir# find . | cpio --quiet --dereference -o -H newc | lzma -7 > ../initrd.lz`
1. Boot w/ the new image and initrd:
1. `(emmentaler1):~$ scp -r root@babybel3:~ /mnt/netos/tftpboot/atriantaf/ubuntu-14.04-custom/initrd-3.13.11-ckt39.lz`
1. `(emmentaler1):~$ scp -r root@babybel3:~ /mnt/netos/tftpboot/atriantaf/ubuntu-14.04-custom/vmlinuz-3.13.11-ckt39`
1. Edit the appropriate fields in `/mnt/netos/tftpboot/atriantaf/ubuntu-14.04-custom.cfg` (kernel, initrd)
rackboot
1. `(babybel3):~/linux-3.13.0/lztempdir# cp -r lib/modules/3.13.11-ckt39 /lib/modules/`
1. `(babybel3):~/linux-3.13.0/lztempdir# cd ..`
1. Check https://unix.stackexchange.com/questions/270123/how-to-create-usr-src-linux-headers-version-files for creating the linux headers.
1. `(babybel3):~/linux-3.13.0# mkdir build`
1. `(babybel3):~/linux-3.13.0# cp /boot/config-3.13.0-24-generic ./build/.config`
1. `(babybel3):~/linux-3.13.0# make mrproper`
1. `(babybel3):~/linux-3.13.0# make O=build/ modules_prepare`
1. `(babybel3):~/linux-3.13.0# cd build`
1. `(babybel3):~/linux-3.13.0/build# make -j 20`
1. `(babybel3):~/linux-3.13.0/build# cd ..`
1. `(babybel3):~/linux-3.13.0# cp -r  build /lib/modules/3.13.11-ckt39/`


## Set up mounting point for persistent storage into emmentaler

1. `(babybel3):~# mkdir tmp_mount`
1. `(babybel3):~# mount -t nfs 10.110.4.4:/mnt/local/nfs/ tmp_mount/  -o relatime,vers=3,rsize=1048576,wsize=1048576,namlen=255,hard,nolock,proto=tcp,port=2049,timeo=7,retrans=3,sec=sys,local_lock=all,addr=10.110.4.4`
1. `(babybel3):~# cd tmp_mount`


## Set up X-Mem in babybel3

1. `(emmentaler1):~$ scp -r ./2021-msc-atriantaf-code/ root@babybel3:~`
1. `(babybel3):~# apt install -y scons`
1. `(babybel3):~# apt install -y libnuma-dev`
1. `(babybel3):~# apt install -y libhugetlbfs-dev`
1. `(babybel3):~# cd 2021-msc-atriantaf-code/X-Mem`
1. `(babybel3):~/2021-msc-atriantaf-code/X-Mem# ./build-linux.sh x64 20`
1. `(babybel3):~/2021-msc-atriantaf-code/X-Mem# ./bin/xmem-linux-x64 --latency_matrix -w40 -n50 --regions=0x1f5713d000,0x3efdb6a000,0x3eaa998000,0x380500000000,0x380100000000 --sync`


## Set up PHI in babybel3

#### (mpss)
1. `(babybel3):~# apt update`
1. `(babybel3):~# apt install -y alien`
1. `(babybel3):~# wget http://registrationcenter-download.intel.com/akdlm/irc_nas/15904/mpss-3.8.6-linux.tar`
1. `(babybel3):~# tar -xvf mpss-3.8.6-linux.tar`
1. `(babybel3):~# cd mpss-3.8.6`
1. `(babybel3):~/mpss-3.8.6# alien --scripts *.rpm`
1. `(babybel3):~/mpss-3.8.6# dpkg -i *.deb`
1. `(babybel3):~/mpss-3.8.6# echo '/usr/lib64' >> /etc/ld.so.conf.d/zz_x86_64-compat.conf`
1. `(babybel3):~/mpss-3.8.6# ldconfig`

#### (kernel module)
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

1. `(babybel3):~# apt install -y scons`
1. `(babybel3):~# echo "ExtraCommandLine nopat" >> /etc/mpss/mic0.conf`
1. `(babybel3):~# echo "ExtraCommandLine nopat" >> /etc/mpss/mic1.conf`
1. `(babybel3):~# micctrl -r -w`
1. `(babybel3):~# micctrl -b`
1. `(babybel3):~# cd X-Mem`
1. replace `gcc` w/ `/usr/linux-k1om-4.7/bin/x86_64-k1om-linux-gcc` in `SConscript`.
1. `(babybel3):~/X-Mem# ./build-linux.sh gcc_mic 20`
1. `(babybel3):~/X-Mem# scp -r ./bin/xmem-linux-gcc_mic mic0:~`
1. `(babybel3):~/X-Mem# cd`
1. `(babybel3):~# ssh mic0`
1. `(mic0):~# ./xmem-linux-gcc_mic -l -r 0x100000000 -n 10`


## Run sockeye-compiler on babybel3 (Ubuntu 14.04)

#### (build cabal)
1. `(babybel3):~# apt remove cabal-install`
1. `(babybel3):~# wget https://downloads.haskell.org/~cabal/cabal-install-1.24.0.2/cabal-install-1.24.0.2-x86_64-unknown-linux.tar.gz`
1. `(babybel3):~# tar -xvf cabal-install-1.24.0.2-x86_64-unknown-linux.tar.gz`
1. `(babybel3):~# export PATH=/root/dist-newstyle/build/x86_64-linux/ghc-8.0.2/cabal-install-1.24.0.2/build/cabal/:$PATH`
1. `(babybel3):~# which cabal`
1. `(babybel3):~# rm /root/.cabal/bin//cabal`
1. `(babybel3):~# which cabal`
1. `(babybel3):~# cabal --version`
1. `(babybel3):~# cabal update`

#### (build ghc)
1. `(babybel3):~# wget https://downloads.haskell.org/~ghc/8.0.1/ghc-8.0.1-x86_64-deb8-linux.tar.xz`
1. `(babybel3):~# tar -xvf ghc-8.0.1-x86_64-deb8-linux.tar.xz`
1. `(babybel3):~# cd ghc-8.0.1/`
1. `(babybel3):~/ghc-8.0.1# ./configure`
1. `(babybel3):~/ghc-8.0.1# make install`
1. `(babybel3):~/ghc-8.0.1# cd`
1. `(babybel3):~# which ghc`
1. `(babybel3):~# ghc --version`
1. `(babybel3):~# sudo apt-get install libgmp3-dev`

1. `(babybel3):~# cabal install aeson`
1. `(babybel3):~# cabal install MissingH`

#### (build eclipseclp)
1. `(babybel3):~# cd 2021-msc-atriantaf-code/sockeye-compiler/eclipse`
1. `(babybel3):~/2021-msc-atriantaf-code/sockeye-compiler/eclipse# mv ./eclipse_basic.tgz ..`
1. `(babybel3):~/2021-msc-atriantaf-code/sockeye-compiler/eclipse# cd ..`
1. `(babybel3):~/2021-msc-atriantaf-code/sockeye-compiler# rm -rf eclipse eclipseclp`
1. `(babybel3):~/2021-msc-atriantaf-code/sockeye-compiler# mkdir eclipse; cd eclipse`
1. `(babybel3):~/2021-msc-atriantaf-code/sockeye-compiler/eclipse# mv ../eclipse_basic.tgz .`
1. `(babybel3):~/2021-msc-atriantaf-code/sockeye-compiler/eclipse# tar -xvf eclipse_basic.tgz`
1. `(babybel3):~/2021-msc-atriantaf-code/sockeye-compiler/eclipse# ./RUNME`
1. `(babybel3):~/2021-msc-atriantaf-code/sockeye-compiler/eclipse# cd ..`
1. `(babybel3):~/2021-msc-atriantaf-code/sockeye-compiler/eclipse# ln -s ./eclipse/bin/x86_64_linux/eclipse eclipseclp`

1. `(babybel3):~/2021-msc-atriantaf-code/sockeye-compiler/eclipse# cd ..`
1. `(babybel3):~/2021-msc-atriantaf-code/sockeye-compiler# make clean`
1. `(babybel3):~/2021-msc-atriantaf-code/sockeye-compiler# make`
1. `(babybel3):~/2021-msc-atriantaf-code/sockeye-compiler# make ECLIPSE=./eclipseclp test_perf`
