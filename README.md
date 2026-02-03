# wiiMac - A Mac OS X Bootloader for the Nintendo Wii

wiiMac enables PowerPC versions of Mac OS X to be booted natively on the Nintendo Wii. To boot Mac OS X, a patched kernel is required.

See [Releases](https://github.com/bryankeller/wiiMac/releases) for a precompiled, ready-to-copy-to-your-SD-card bootloader binary.

<Image of running system></>

## Supported Mac OS X Versions

| Mac OS X Version | Patched Kernel  | USB |
|:----|:---:|:---:|
| 10.0 Cheetah  | [xnu-124.13-wii](https://github.com/bryankeller/xnu-124.13-wii) | [IOUSBFamily-1.8.2-wii](https://github.com/bryankeller/IOUSBFamily-1.8.2-wii) |
| 10.1 Puma | - | - |
| 10.2 Jaguar | - | - |
| 10.3 Panther | - | - |
| 10.4 Tiger | - | - |
| 10.5 Leopard | - | - |

### Limitations

The following hardware is not yet supported:

- Wi-Fi
- Bluetooth
- DVD drive
- Hardware-accelerated graphics
- Audio

## Installation Guide

### Prerequisites

To use wiiMac, you’ll need:

- A Wii with an SD card slot (the Wii Mini is not supported)
- A Wii that’s been soft-modded, with [BootMii](https://bootmii.org/about/) [installed](https://bootmii.org/install/) as boot2 or IOS
- A [MBR](https://en.wikipedia.org/wiki/Master_boot_record)-formatted SD card with a FAT32 partition containing BootMii files (/bootmii/ppcboot.elf and /bootmii/armboot.bin) 
- A second SD card that’s at least 4 GB for the Mac OS X system

If you can open the BootMii menu, you’re ready to set up wiiMac. If you can’t, follow the instructions [here](https://consolemods.org/wiki/Wii:Introduction_to_Wii_Softmodding).

<BootMii screenshot></>

### SD Card Setup

As noted in the prerequisites above, two SD cards are needed.

<details>
<summary>Advanced users</summary>

Technically, you can get away with one SD card using a hybrid partitioning scheme: sector 0 contains MBR data, and sector 1 contains the APM data. This approach enables you to have the BootMii, wiiMac files and the installed Mac OS X system all on the same SD card. Exactly how to accomplish this is left as an exercise for advanced users.

</details>

#### BootMii SD Card

First, we’ll install the wiiMac bootloader onto the SD card that you used to open the BootMii menu (see [prerequisites](#Prerequisites)).

1. Copy the entire `wiiMac` folder (containing `wiiMac.elf` and `wiiMac.config`) to the root of your SD card.

2. Verify that the following files exist:
```
/bootmii/ppcboot.elf
/bootmii/armboot.bin
/wiiMac/wiiMac.elf
/wiiMac/wiiMac.config
```

3. Set the correct `video_mode` for your Wii in `/wiiMac/wiiMac.config` (ntscp, ntsci, pal60, pal50) 

#### Mac OS X System SD Card

Next, _on a different SD card_, we’ll prepare a Mac OS X system to boot. The end-goal of this section is to have an APM-formatted SD card with a bootable Mac OS X system on an HFS+ partition.

If you already have a disk image of a fully-installed, [supported system](#Supported-Mac-OS-X-Versions), you can use `dd` commands or [balenaEtcher](https://etcher.balena.io) to flash the image to your SD card, bypassing the manual installation steps below. You can obtain a disk image for a supported system from archive.org, or make your own using QEMU to emulate Mac OS X. You can probably even image the hard drive or a real Mac computer, although I haven’t tested that approach.

If you don’t have a ready-to-flash image, you’ll need to manually partition your SD card and prepare an installer yourself.

##### Partitioning

Partitioning instructions depend on what host operating system you’re running.

<details>
<summary>macOS Host</summary>

1. Run the following terminal command to obtain the device number for the target SD card:
```
diskutil list
```

2. Partition the SD card. Warning: this will erase the SD card.
```
# Replace diskN with the correct device number for the SD card
diskutil partitionDisk diskN APM HFS+ "Mac" R HFS+ "Install" 2G
```

</details>

<details>
<summary>Linux Host</summary>

1. Run the following terminal command to obtain the device for the target SD card:
```
lsblk
```

2. Partition the SD card. Warning: this will erase the SD card.
```
# Replace sdX with the correct device for the SD card
sudo parted /dev/sdX --script mklabel mac mkpart primary hfs+ 1MiB -2GiB mkpart primary hfs+ -1GiB 100%
```

3. Format the partitions:
```
# If hfsprogs is not installed:
sudo apt install hfsprogs        # Debian / Ubuntu
sudo pacman -S hfsprogs          # Arch

# then

# Replace sdX with the correct device for the SD card
sudo mkfs.hfsplus -v Mac /dev/sdX1
sudo mkfs.hfsplus -v Install /dev/sdX2
```

</details>

<details>
<summary>Windows Host</summary>

Partitioning an APM disk on Windows is difficult. Use a Linux VM or find a ready-to-flash system image and use [balenaEtcher](https://etcher.balena.io) to flash the image to your SD card.

</details>

##### Flashing Installer

Next, we need our newly-created Install partition to contain a bootable Mac OS X installer. ISO backups of Mac OS X install media exists for many versions of Mac OS X. Once you’ve obtained an installer disk image for a [supported version](#Supported-Mac-OS-X-Versions), mount it and then perform the following steps depending on your host operating system:

<details>
<summary>macOS Host</summary>

1. Run the following terminal command to obtain the device numbers for the SD card Install partition and the source installation partition.
```
diskutil list
```

2. Unmount both partitions:
```
# Replace diskNsX with the correct device and partition number for the SD card Install partition
diskutil unmount diskNsX
# Replace diskMsY with the correct device and partition number for the source installation
diskutil unmount diskMsY
```

3. Block-level copy the contents of the source installation partition to the destination SD card partition:
```
# Replace diskNsY with the correct device and partition number for the SD card Install partition
# Replace diskMsY with the correct device and partition number for the source installation
sudo dd if=/dev/rdiskNsX of=/dev/rdiskMsY bs=512k status=progress
```

</details>