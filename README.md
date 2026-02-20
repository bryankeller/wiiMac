# wiiMac - A Mac OS X Bootloader for the Nintendo Wii

wiiMac enables PowerPC versions of Mac OS X to be booted natively on the Nintendo Wii.

See [Releases](https://github.com/bryankeller/wiiMac/releases) for a precompiled, ready-to-run bootloader executable.

![Image of Mac OS X Cheetah running natively on the Nintendo Wii](/assets/IMG_9159.jpeg)

## Supported Mac OS X Versions

| Mac OS X Version | Patched Kernel | Drivers |
|:----|:---:|:---:|
| 10.0 Cheetah  | [wii-xnu-124.13](https://github.com/bryankeller/wii-xnu-124.13) | [wii-macosx-cheetah-drivers](https://github.com/bryankeller/wii-macosx-cheetah-drivers) |
| 10.1 Puma | - | - |
| 10.2 Jaguar | - | - |
| 10.3 Panther | - | - |
| 10.4 Tiger | - | - |
| 10.5 Leopard | - | - |

### Limitations

The following hardware is not yet supported:

- Wi-Fi
- Bluetooth
- Optical drive
- Hardware-accelerated graphics
- Audio

## Installation Guide

### Prerequisites

To use wiiMac, you’ll need:

- A Wii with an SD card slot (the [Wii Mini](https://wiibrew.org/wiki/Wii_mini) is not supported)
- A Wii that’s been soft-modded, with [BootMii](https://bootmii.org/about/) [installed](https://bootmii.org/install/) as boot2 or IOS
- A [MBR](https://en.wikipedia.org/wiki/Master_boot_record)-formatted SD card with a FAT32 partition containing BootMii files (/bootmii/ppcboot.elf and /bootmii/armboot.bin) 
- A second SD card that’s at least 4 GB for the Mac OS X system

If you can open the BootMii menu, you’re ready to set up wiiMac. If you can’t, follow the instructions [here](https://consolemods.org/wiki/Wii:Introduction_to_Wii_Softmodding).

![Image of BootMii running](/assets/bootmii.png)

### SD Card Setup

As noted in the prerequisites above, two SD cards are needed.

<details>
<summary>Advanced users</summary>

Technically, you can get away with one SD card using a hybrid partitioning scheme: sector 0 contains MBR data, and sector 1 contains the APM data. This approach enables you to have the BootMii, wiiMac files, and the installed Mac OS X system all on the same SD card. Exactly how to accomplish this is left as an exercise for advanced users, but the gist of it is that you need to use `fdisk` to manually create an MBR at sector 0.

</details>

#### BootMii SD Card

First, we’ll install the wiiMac bootloader onto the SD card that you used to open the BootMii menu (see [prerequisites](#Prerequisites)).

1. Download the latest version from [Releases](https://github.com/bryankeller/wiiMac/releases)

2. Copy the entire `wiiMac` folder (containing `wiiMac.elf` and `config.txt`) to the root of your SD card

3. Verify that the following files exist:
```
/
└── bootmii
    ├── ppcboot.elf
    └── armboot.bin
└── wiiMac
    ├── wiiMac.elf
    └── config.txt
```

4. Set the correct `video_mode` for your Wii in `/wiiMac/config.txt` (ntscp, ntsci, pal60, pal50) 

#### Mac OS X System SD Card

Next, _on a different SD card_, we’ll prepare a Mac OS X system to be installed. We’ll need three partitions: a destination partition to which we’ll install Mac OS X, an install partition that we’ll boot from to run the installer, and a support partition containing a patched kernel and drivers.

##### Partitioning

Partitioning instructions depend on which host operating system you’re running.

<details>
<summary>macOS Host</summary>

1. Run the following terminal command to obtain the device for the target SD card:
```
diskutil list
```

2. Partition the SD card. Warning: this will erase the SD card.
```
# Replace diskX with the correct device for the SD card
diskutil partitionDisk diskX APM \
  HFS+ "Macintosh HD" R \
  HFS+ "Install" 1G\
  FAT32 "Support" 64M
```

</details>

<details>
<summary>Linux Host</summary>

1. Run the following terminal command to obtain the device for the target SD card:
```
lsblk -f
```

2. Partition the SD card. Warning: this will erase the SD card.
```
# Replace sdX with the correct device for the SD card
sudo parted /dev/sdX --script \
  mklabel mac \
  mkpart primary hfs+ 1MiB -1088MiB \
  mkpart primary hfs+ -1088MiB -64MiB \
  mkpart primary fat32 -64MiB 100%
```

3. Format the partitions:
```
# If hfsprogs is not installed:
sudo apt install hfsprogs        # Debian / Ubuntu
sudo pacman -S hfsprogs          # Arch

# then

# Replace sdX with the correct device for the SD card
sudo mkfs.hfsplus -v "Macintosh HD" /dev/sdX1
sudo mkfs.hfsplus -v "Install" /dev/sdX2
sudo mkfs.vfat -F 32 -n "Support" /dev/sdX3
```

</details>

##### Flashing Installer

Next, we need our newly-created Install partition to contain a bootable Mac OS X installer. ISO backups of Mac OS X install media exist for many versions of Mac OS X. Once you’ve obtained an installer disk image for a [supported version](#Supported-Mac-OS-X-Versions), mount it and then perform the following steps depending on your host operating system:

<details>
<summary>macOS Host</summary>

1. Run the following terminal command to obtain the device and partition numbers for the SD card Install partition and the Mac OS X installer partition.
```
diskutil list
```

2. Unmount both partitions:
```
# Replace diskXsA with the correct device and partition number for the SD card Install partition
diskutil unmount diskXsA
# Replace diskYsB with the correct device and partition number for the Mac OS X installer partition
diskutil unmount diskYsB
```

3. Block-level copy the contents of the source Mac OS X installer partition to the destination Install partition on your SD card:
```
# Replace diskXsA with the correct device and partition number for the SD card Install partition
# Replace diskMsY with the correct device and partition number for the source installation
sudo dd if=/dev/rdiskYsB of=/dev/rdiskXsA bs=512k status=progress
```

</details>

<details>
<summary>Linux Host</summary>

1. Run the following terminal command to obtain the device and partition numbers for the SD card Install partition and the Mac OS X installer partition.
```
lsblk -f
```

2. Unmount both partitions:
```
# Replace sdXA with the correct device and partition number for the SD card Install partition
sudo umount /dev/sdXA
# Replace sdYB with the correct device and partition number for the Mac OS X installer partition
sudo umount /dev/sdYB
```

3. Block-level copy the contents of the source Mac OS X installer partition to the destination Install partition on your SD card:
```
# Replace sdXA with the correct device and partition number for the SD card Install partition
# Replace sdYB with the correct device and partition number for the source installation
sudo dd if=/dev/sdYB of=/dev/sdXA bs=1M status=progress conv=fsync
```

</details>

##### Preparing Support Partition

The FAT32 “Support” partition is where we’ll store the patched kernel and drivers. Create a folder titled “wiiMac” at the root of this partition, and copy the appropriate mach_kernel and driver kexts into that folder. Refer to the [supported version table](#Supported-Mac-OS-X-Versions) for links to download a patched kernel and drivers.

Verify that the following files exist on the Support partition:

```
/
└── wiiMac
    ├── mach_kernel
    ├── IOUSBFamily.kext
    └── NintendoWii*.kext (all other drivers)
```

### Installing Mac OS X

Now that we have both of our SD cards set up, we’re ready to boot into the Mac OS X installer to install Mac OS X.

#### Running wiiMac

First, we need to load and run the wiiMac bootloader using BootMii.

1. Insert the BootMii SD card and load into the BootMii menu
2. Navigate to the SD card icon and select it
3. Navigate to the wiiMac folder, and open wiiMac.elf

You should now be running the wiiMac bootloader.

![wiiMac Bootloader Running](/assets/wiiMac.png)

#### Booting the Mac OS X Installer

Next, we’ll boot the Mac OS X installer so that we can install Mac OS X.

1. Remove the BootMii SD card
2. Insert the Mac OS X System SD card
3. Use the Power button to select to the Mac OS X installer partition
4. Press the Reset button to boot the Mac OS X installer

The Mac OS X installer will now load. Use a USB mouse and keyboard to navigate the interface and install Mac OS X to “Macintosh HD”. Your Wii will restart when the installation completes.

#### Booting Mac OS X

Now that Mac OS X is installed, we can boot to a fully-usable system.

1. Remove the Mac OS X System SD card
2. Insert the BootMii SD card and load into the BootMii menu
3. Navigate to the SD card icon and select it
4. Navigate to the wiiMac folder, and open wiiMac.elf
5. Remove the BootMii SD card
6. Insert the Mac OS X System SD card
7. Use the Reset button to select to the Macintosh HD partition
8. Use the Eject button to override boot arguments to include Force800x600=1
9. Press the Reset button to boot Mac OS X

After booting for the first time, Mac OS X will walk you through a setup process. The setup process requires at least 800x600 resolution - higher than the Wii natively supports. By using the “Force800x600=1” boot argument, we can temporarily force the Wii to render into a larger framebuffer, allowing us to complete the setup process. While operating in this mode, visual quality is significantly reduced.

After completing the initial setup process, you should be on the Mac OS X desktop.

### Next Steps

#### System Optimization

First, I'd highly recommend opening System Preferences > Display, and reducing the resolution back to 640x480 - the native output resolution of the Wii. Doing so will significantly improve visual quality.

Next, I’d recommend going to System Preferences > Dock, and reducing the size of the Dock or turning Dock hiding on - we need every bit of space we can get on the 640x480 video output.

Lastly, to improve system responsiveness, I’d also recommend changing the Swap file size from ~76MB down to ~7.6MB. You can do that by editing this line in `/etc/rc` from:
`dynamic_pager -H 40000000 -L 160000000 -S 80000000 -F ${swapdir}/swapfile`
to
`dynamic_pager -H 40000000 -L 160000000 -S 8000000 -F ${swapdir}/swapfile`
After, reboot the system for changes to take effect.

#### Getting Software

Useful software can be found at Archive.org and https://www.macintoshrepository.org. Here are some that I’ve tried:

- [Mac OS 9.1 System Folder to enable Classic apps to run](https://www.macintoshrepository.org/35900-mac-os-9-1-system-folder-for-mac-os-x-classic-environment-)
- [Mac OS X 10.0 Developer Tools](https://www.macintoshrepository.org/15940-developer-tools-for-mac-os-x-10-0)
- [Microsoft Office 2001](https://www.macintoshrepository.org/2731-microsoft-office-2001) (requires Classic)
- [Oregon Trail](https://www.macintoshrepository.org/5779-the-oregon-trail) (requires Classic)