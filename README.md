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
sudo dnf install hfsplus-tools   # Fedora

# then

# Replace sdX with the correct device for the SD card
sudo mkfs.hfsplus -v "Macintosh HD" /dev/sdX2
sudo mkfs.hfsplus -v "Install" /dev/sdX3
sudo mkfs.vfat -F 32 -n "Support" /dev/sdX4
```

</details>

<details>
<summary>Windows Subsystem for Linux (WSL) Host</summary>

**Mount Media**
To use WSL to prepare media, we need to expose USB devices to WSL.

1. Install usbipd-win:

```batch
winget install --interactive --exact dorssel.usbipd-win
```

2. Open a new PowerShell window as **Administrator** and list USB devices without the SD card inserted:

```batch
usbipd list
```

```
Connected:
BUSID  VID:PID    DEVICE                                             STATE
5-3    0bda:8156  Realtek USB 2.5GbE Family Controller               Not shared
9-4    0bda:1101  USB Input Device                                   Not shared
10-1   045e:0772  Microsoft® LifeCam Studio(TM), USB Input Device    Not shared
10-2   045e:077b  USB Input Device                                   Not shared
10-4   0b0e:0412  Jabra SPEAK 410 USB, USB Input Device              Not shared
10-5   0bda:1100  USB Input Device                                   Not shared

Persisted:
GUID                                  DEVICE
890b7cb4-6195-472d-9326-407f4c6303a3  USB Mass Storage Device
```

3. Plug in the SD card and re-enumerate USB devices to identify the SD card (7-1 in the output below)

```batch
usbipd list
```

```
Connected:
BUSID  VID:PID    DEVICE                                             STATE
5-3    0bda:8156  Realtek USB 2.5GbE Family Controller               Not shared
7-1    05e3:0749  USB Mass Storage Device                            Shared
9-4    0bda:1101  USB Input Device                                   Not shared
10-1   045e:0772  Microsoft® LifeCam Studio(TM), USB Input Device    Not shared
10-2   045e:077b  USB Input Device                                   Not shared
10-4   0b0e:0412  Jabra SPEAK 410 USB, USB Input Device              Not shared
10-5   0bda:1100  USB Input Device                                   Not shared

Persisted:
GUID                                  DEVICE
890b7cb4-6195-472d-9326-407f4c6303a3  USB Mass Storage Device
```

4. Prepare to share the device (7-1):

```batch
usbipd bind --busid 7-1
```

5. Attach the device to WSL:

```batch
usbipd attach --wsl --busid 7-1
```

6. Run the following terminal command to obtain the device for the target SD card:

```bash
lsblk -f
```

```
NAME
    FSTYPE FSVER LABEL UUID                                 FSAVAIL FSUSE% MOUNTPOINTS
sda ext4   1.0
sdb ext4   1.0
sdc swap   1           3d051596-f50e-407d-9696-a285606a071c                [SWAP]
sdd ext4   1.0         633fba1b-1ccb-4dbf-bed6-79822f9f35a1  954.1G     0% /mnt/wslg/distro
                                                                           /
sde
sdf
```

`sde` and `sdf` are the volumes for the SD Card (and adapter?). 

7. Install needed utilities for disk partitioning and formatting:

```bash
sudo apt install parted hfsprogs dosfstools mtools kpartx
```

8. Partition the SD card. Warning: this will erase the SD card.

```bash
# Replace sdX with the correct device for the SD card
sudo 
parted /dev/sdX
mklabel mac
mkpart primary hfs+ 1MiB -1088MiB
mkpart primary hfs+ -1088MiB -64MiB
mkpart primary fat32 -64MiB 100%
quit
```

9. Format the partitions (still as sudo)

```bash
# Replace sdX with the correct device for the SD card
mkfs.hfsplus -v "Macintosh HD" /dev/sdX2
mkfs.hfsplus -v "Install" /dev/sdX3
mkfs.vfat -F 32 -n "Support" /dev/sdX4
```

The partitions should look like this:

```bash
lsblk -f
NAME   FSTYPE  FSVER LABEL        UUID                                 FSAVAIL FSUSE% MOUNTPOINTS
sda    ext4    1.0
sdb    ext4    1.0
sdc    swap    1                  3d051596-f50e-407d-9696-a285606a071c                [SWAP]
sdd    ext4    1.0                633fba1b-1ccb-4dbf-bed6-79822f9f35a1  953.5G     0% /mnt/wslg/distro
                                                                                      /
sde
├─sde1
├─sde2 hfsplus       Macintosh HD 5597f954-c9f4-36fc-b381-fe2ff0f36980
├─sde3 hfsplus       Install      2af72876-9f6e-3364-bbe4-6cc68c3abcaa
├─sde4 vfat    FAT32 Support      6852-6DC8
└─sde5
sdf
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

1. Use the following command to create logical devices for each partition in the installer image.
```
sudo kpartx -av /path/to/installer/iso
```

2. Run the following terminal command to obtain the device and partition numbers for the SD card Install partition and the Mac OS X installer partition.
```
lsblk -f
```

3. Unmount both partitions:
```
# Replace sdXA with the correct device and partition number for the SD card Install partition
sudo umount /dev/sdXA
# Replace sdYB with the correct device and partition number for the Mac OS X installer partition
sudo umount /dev/sdYB
```

4. Block-level copy the contents of the source Mac OS X installer partition to the destination Install partition on your SD card:
```
# Replace sdXA with the correct device and partition number for the SD card Install partition
# Replace sdYB with the correct device and partition number for the source installation
sudo dd if=/dev/sdYB of=/dev/sdXA bs=1M status=progress conv=fsync
```

</details>

<details>
<summary>WSL Host</summary>

1. Download the ISO in Windows and copy into WSL.

2. Use the following commands to create logical devices for each partition in the installer image.

```bash
sudo kpartx -av /path/to/installer/iso
```

Example output:

```
add map loop0p3 (254:0): 0 60 linear 7:0 4
add map loop0p4 (254:1): 0 56 linear 7:0 64
add map loop0p5 (254:2): 0 120 linear 7:0 120
add map loop0p6 (254:3): 0 56 linear 7:0 240
add map loop0p7 (254:4): 0 120 linear 7:0 296
add map loop0p8 (254:5): 0 512 linear 7:0 416
add map loop0p9 (254:6): 0 1157000 linear 7:0 928
```

2. Run the following terminal command to obtain the device and partition numbers for the SD card Install partition and the Mac OS X installer partition.

```bash
lsblk -f
```

Example output:

```
NAME      FSTYPE  FSVER LABEL               UUID                                 FSAVAIL FSUSE% MOUNTPOINTS
loop0
├─loop0p3
├─loop0p4
├─loop0p5
├─loop0p6
├─loop0p7
├─loop0p8
└─loop0p9 hfsplus       Mac OS X Install CD 8b4e037f-d046-3610-a311-757aaede1493
sda       ext4    1.0
sdb       ext4    1.0
sdc       swap    1                         3d051596-f50e-407d-9696-a285606a071c                [SWAP]
sdd       ext4    1.0                       633fba1b-1ccb-4dbf-bed6-79822f9f35a1  953.5G     0% /mnt/wslg/distro
                                                                                                /
sde
├─sde1
├─sde2    hfsplus       Macintosh HD        5597f954-c9f4-36fc-b381-fe2ff0f36980
├─sde3    hfsplus       Install             2af72876-9f6e-3364-bbe4-6cc68c3abcaa
├─sde4    vfat    FAT32 Support             6852-6DC8
└─sde5
sdf
```

3. Block-level copy the contents of the source Mac OS X installer partition to the destination Install partition on your SD card.

> Note that the loopback partitions are exposed under /dev/mapper and not /dev/:

```bash
# Replace sdXA with the correct device and partition number for the SD card Install partition
# Replace loopYpB with the correct device and partition number for the source installation
sudo dd if=/dev/mapper/loopYpB of=/dev/sdXA bs=1M status=progress conv=fsync
```

Example:

```
sudo dd if=/dev/mapper/loop0p9 of=/dev/sde3 bs=1M status=progress conv=fsync
307232768 bytes (307 MB, 293 MiB) copied, 1 s, 307 MB/s592384000 bytes (592 MB, 565 MiB) copied, 1.86659 s, 317 MB/s

564+1 records in
564+1 records out
592384000 bytes (592 MB, 565 MiB) copied, 77.7162 s, 7.6 MB/s
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

<details>
<summary>WSL Host Notes</summary>

Microsoft's default WSL2 kernel is heavily optimized and intentionally omits the `vfat` (FAT32) kernel module to save space and reduce boot time. Because of this, the standard `mount` command for FAT32 filesystems will always fail natively inside WSL2 unless you compile a custom kernel. You can attempt to unmount the volume and just copy the files in using Windows but that didn't work for me.  Instead we'll use ```mtools``` utilities to copy files into the partition.

```bash
sudo mmd -i /dev/sdXA ::/wiiMac
sudo mcopy -s -i /dev/sdXA /mnt/c/path/to/files/* ::/wiiMac
```

Confirm the contents of the drive:

```bash
sudo mdir -i /dev/sdXA ::/wiiMac/
```

Example output:

```
 Volume in drive : is Support
 Volume Serial Number is 6852-6DC8
Directory for ::/wiiMac

.            <DIR>     2026-06-28  19:28
..           <DIR>     2026-06-28  19:28
IOUSBF~1 KEX <DIR>     2026-06-28  19:03  IOUSBFamily.kext
NINTEN~1 KEX <DIR>     2026-06-28  19:03  NintendoWiiEXI.kext
NINTEN~2 KEX <DIR>     2026-06-28  19:03  NintendoWiiFramebuffer.kext
NINTEN~3 KEX <DIR>     2026-06-28  19:03  NintendoWiiHollywood.kext
NINTEN~4 KEX <DIR>     2026-06-28  19:03  NintendoWiiIPC.kext
NINTEN~5 KEX <DIR>     2026-06-28  19:03  NintendoWiiNVRAM.kext
NINTEN~6 KEX <DIR>     2026-06-28  19:03  NintendoWiiPE.kext
NINTEN~7 KEX <DIR>     2026-06-28  19:03  NintendoWiiPIC0.kext
NINTEN~8 KEX <DIR>     2026-06-28  19:03  NintendoWiiPIC1.kext
NINTEN~9 KEX <DIR>     2026-06-28  19:03  NintendoWiiSDCard.kext
MACH_K~1       4038868 2026-06-28  19:03  mach_kernel
       13 files           4 038 868 bytes
                         57 349 120 bytes free
```

Then unmount the drive from WSL in your Administrator PowerShell window:

```powershell
usbipd detach --all
```

</details>

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

First, I'd highly recommend opening System Preferences > Display, and reducing the resolution back to 640x480 - doing so will significantly improve visual quality.

Next, I’d recommend going to System Preferences > Dock, and reducing the size of the Dock or turning Dock hiding on - we need every bit of space we can get on the 640x480 video output.

Lastly, to improve system responsiveness, I’d also recommend changing the Swap file size from ~76MB down to ~7.6MB. You can do that by editing this line in `/etc/rc` from:
`dynamic_pager -H 40000000 -L 160000000 -S 80000000 -F ${swapdir}/swapfile`
to
`dynamic_pager -H 40000000 -L 160000000 -S 8000000 -F ${swapdir}/swapfile`
After, reboot the system for changes to take effect.

#### Getting Software

Useful software can be found at Archive.org and https://www.macintoshrepository.org. Here are some that I’ve tried:

- [10.0.4 update](https://download.info.apple.com/Mac_OS_X/062-8263.20010621/1z/Mac_OS_X_10.0.4_UpdateCom.dmg.bin)
- [Mac OS 9.1 System Folder to enable Classic apps to run](https://www.macintoshrepository.org/35900-mac-os-9-1-system-folder-for-mac-os-x-classic-environment-)
- [Mac OS X 10.0 Developer Tools](https://www.macintoshrepository.org/15940-developer-tools-for-mac-os-x-10-0)
- [Microsoft Office 2001](https://www.macintoshrepository.org/2731-microsoft-office-2001) (requires Classic)
- [Oregon Trail](https://www.macintoshrepository.org/5779-the-oregon-trail) (requires Classic)
