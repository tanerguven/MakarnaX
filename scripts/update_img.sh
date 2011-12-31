#!/bin/bash
#
# http://www.andremiller.net/content/mounting-hard-disk-image-including-partitions-using-linux
#
sudo mount -o loop,rw,offset=512 bin/kernel.img bin/mnt
sleep 0.1
sudo cp bin/kernel bin/mnt/kernel
sleep 0.1
sudo umount bin/mnt
