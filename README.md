# ElasticFramework

## Install AMD Graphic Driver
```
$sudo add-apt-repository ppa:xorg-edgers/ppa
$sudo apt-get update
$sudo apt-get install fglrx
$sudo amdconfig --initial
$sudo apt-get install gksu
```
## Install Nvidia Driver

```

sudo add-apt-repository ppa:graphics-drivers

sudo apt-get update

sudo apt-get install nvidia-355

sudo reboot
lsmod | grep nvidia
```
## install cppjson
