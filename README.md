Ladybug installation
====================
**Recommended OS: Ubuntu 16.04**

## Table of Contents:

1) [How to install Ladybug on Ubuntu 16.04](#how-to-install-ladybug-on-Ubuntu-16.04)
2) [Post-installation](#post-installation)
3) [Additional libraries and running the examples](#additional-libraries-and-running-the-examples)
4) [Solving dependencies](#solving-dependencies)
5) [Uninstalling](#uninstalling)
6) [Important directories](#important-directories)


## How to install Ladybug on Ubuntu 16.04

1) Requirements:

```shell
sudo dpkg -i libxerces-c3.1_3.1.3+debian-1_amd64.deb
```

```shell
sudo apt-get install xsdcxx
```

2) Extract all files from LadybugSDK_1.16.3.48_Ubuntu16.04_amd64.tar:

```shell
tar -xf LaydbugSDK_1.16.3.48_Ubuntu16.04_amd64.tar
```

3) Install ladybug-1.16.3.48_amd64.deb:

```shell
sudo dpkg -i ladybug-1.16.3.48_amd64.deb
```

## Post-installation

1) Open the /etc/default/grub file in any text editor.

2) Replace:
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash"

3) With:
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash usbcore.usbfs_memory_mb=1000"

4) Update and reboot:

```shell
sudo update-grub

sudo reboot
```

5) Check that it is enabled:

```shell
cat /sys/module/usbcore/parameters/usbfs_memory_mb
```

## Additional libraries and running the examples

1) Install the additional libraries:

```shell
sudo apt-get update

sudo apt-get install freeglut3 freeglut3-dev -y

sudo apt-get install libswscale-dev libavcodec-dev libavformat-dev -y
```

2) Creating executable:

```shell
$USER@$HOSTNAME:/usr/src/ladybug/src/ladybugAdvancedRenderEx$ sudo make
[sudo] password for $USER: 
Creating executable
g++ -Wl,--exclude-libs=ALL -o LadybugAdvancedRenderEx obj/ladybugAdvancedRenderEx.o -L../../lib -L/usr/lib/ladybug -lflycapture -lladybug -lptgreyvideoencoder -lGLU -lGL -lglut
```

## Solving dependencies

During the installation, some dependencies could be required.

1) libicu55

```shell
sudo add-apt-repository "deb http://security.ubuntu.com/ubuntu xenial-security main"
sudo apt-get update
sudo apt-get install libicu55
```

## Uninstalling

The package should be installed as a system package:

```shell
sudo apt-get remove ladybug
```

## Important directories

```shell
/etc/ladybug
/lib/ladybug
/usr/src/ladybug		-> Examples
/usr/share/doc/ladybug	-> Documentation
/usr/include/ladybug	-> PATH
/usr/lib/ladybug
```
