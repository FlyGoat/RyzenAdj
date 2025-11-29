# RyzenAdj
Adjust power management settings for Ryzen Mobile Processors.

[![Build Status](https://travis-ci.org/FlyGoat/RyzenAdj.svg?branch=master)](https://travis-ci.org/FlyGoat/RyzenAdj)

Based on: [FlyGoat/ryzen_nb_smu](https://github.com/flygoat/ryzen_nb_smu)

RyzenAdjUI_WPF by "JustSkill" is no longer maintained, for GUI please see  [Universal x86 Tuning Utility](https://github.com/JamesCJ60/Universal-x86-Tuning-Utility) or [ryzen-controller-team/ryzen-controller](https://gitlab.com/ryzen-controller-team/ryzen-controller/).

## Usage
The command line interface is identical on both Windows and Unix-Like OS.

You should run it with Administrator on Windows or root on Linux.

You can write a shell script or bat to do it automatically.

```
$./ryzenadj -h
Usage: ryzenadj [options]

 Ryzen Power Management adjust tool.

    -h, --help                            show this help message and exit

Options
    -i, --info                            Show information and most important power metrics after adjustment
    --dump-table                          Show whole power metric table before and after adjustment

Settings
    -a, --stapm-limit=<u32>               Sustained Power Limit         - STAPM LIMIT (mW)
    -b, --fast-limit=<u32>                Actual Power Limit            - PPT LIMIT FAST (mW)
    -c, --slow-limit=<u32>                Average Power Limit           - PPT LIMIT SLOW (mW)
    -d, --slow-time=<u32>                 Slow PPT Constant Time (s)
    -e, --stapm-time=<u32>                STAPM constant time (s)
    -f, --tctl-temp=<u32>                 Tctl Temperature Limit (degree C)
    -g, --vrm-current=<u32>               VRM Current Limit             - TDC LIMIT VDD (mA)
    -j, --vrmsoc-current=<u32>            VRM SoC Current Limit         - TDC LIMIT SoC (mA)
    -k, --vrmmax-current=<u32>            VRM Maximum Current Limit     - EDC LIMIT VDD (mA)
    -l, --vrmsocmax-current=<u32>         VRM SoC Maximum Current Limit - EDC LIMIT SoC (mA)
    -m, --psi0-current=<u32>              PSI0 VDD Current Limit (mA)
    -n, --psi0soc-current=<u32>           PSI0 SoC Current Limit (mA)
    -o, --max-socclk-frequency=<u32>      Maximum SoC Clock Frequency (MHz)
    -p, --min-socclk-frequency=<u32>      Minimum SoC Clock Frequency (MHz)
    -q, --max-fclk-frequency=<u32>        Maximum Transmission (CPU-GPU) Frequency (MHz)
    -r, --min-fclk-frequency=<u32>        Minimum Transmission (CPU-GPU) Frequency (MHz)
    -s, --max-vcn=<u32>                   Maximum Video Core Next (VCE - Video Coding Engine) (MHz)
    -t, --min-vcn=<u32>                   Minimum Video Core Next (VCE - Video Coding Engine) (MHz)
    -u, --max-lclk=<u32>                  Maximum Data Launch Clock (MHz)
    -v, --min-lclk=<u32>                  Minimum Data Launch Clock (MHz)
    -w, --max-gfxclk=<u32>                Maximum GFX Clock (MHz)
    -x, --min-gfxclk=<u32>                Minimum GFX Clock (MHz)
    -y, --prochot-deassertion-ramp=<u32>  Ramp Time After Prochot is Deasserted: limit power based on value, higher values does apply tighter limits after prochot is over
    --apu-skin-temp=<u32>                 APU Skin Temperature Limit    - STT LIMIT APU (degree C)
    --dgpu-skin-temp=<u32>                dGPU Skin Temperature Limit   - STT LIMIT dGPU (degree C)
    --apu-slow-limit=<u32>                APU PPT Slow Power limit for A+A dGPU platform - PPT LIMIT APU (mW)
    --skin-temp-limit=<u32>               Skin Temperature Power Limit (mW)
    --power-saving                        Hidden options to improve power efficiency (is set when AC unplugged): behavior depends on CPU generation, Device and Manufacture
    --max-performance                     Hidden options to improve performance (is set when AC plugged in): behavior depends on CPU generation, Device and Manufacture
```

### Demo
If I'm going to set all the Power Limit to 45W, and Tctl to 90 °C,
then the command line should be:

    ./ryzenadj --stapm-limit=45000 --fast-limit=45000 --slow-limit=45000 --tctl-temp=90

### Documentation
- [Supported Models](https://github.com/FlyGoat/RyzenAdj/wiki/Supported-Models)
- [Renoir Tuning Guide](https://github.com/FlyGoat/RyzenAdj/wiki/Renoir-Tuning-Guide)
- [Options](https://github.com/FlyGoat/RyzenAdj/wiki/Options)
- [FAQ](https://github.com/FlyGoat/RyzenAdj/wiki/FAQ)

## Installation

You don't need to install RyzenAdj because it does not need configuration, everything is set via arguments
However, some settings could get overwritten by power management features of your device, and you need to regularly set your values again.

We did provide some examples for automation. And these require configuration during installation.

### Linux Installation

Because it is very easy to build the latest version of RyzenAdj on Linux, we don't provide precompiled packages for distributions.
Just follow the build instructions below and you are ready to use it.

### Windows Installation

Before you start installing anything, it is highly recommended getting familiar with RyzenAdj to find out what can be done on your device.
Use the CLI `ryzenadj.exe` to test the support of your device and to benchmark the effects of each setting.
If your values don't stay persistent you may want to consider installing our example script for automation.

1. Prepare your favorite RyzenAdj arguments
1. Copy the content of your RyzenAdj folder to the final destination
1. Put your configuration into `readjustService.ps1` and test it as administrator until everything works as expected
1. Install `readjustService.ps1` as Task for Windows Task Scheduler by running `installServiceTask.bat`

Deinstallation of the Task can be done via `uninstallServiceTask.bat`

Over Windows Task Scheduler you can check if it is running. It is called `RyzenAdj` below `AMD` folder.
Or just run

    SCHTASKS /query /TN "AMD\RyzenAdj"

## Build

### Build Requirements

Building this tool requires C & C++ compilers as well as **cmake**.

### Linux

RyzenAdj needs elevated access to the NB config space. This can be achieved by using either one of
these two methods:

* Using libpci and exposing `/dev/mem`
* Using the ryzen\_smu kernel module

RyzenAdj will try ryzen\_smu first, and then fallback to /dev/mem, if no compatible smu driver is found.
The minimum supported version of ryzen_smu is 0.1.7
If no backend is available, RyzenAdj will fail initialization.

_**Please note that `/dev/mem` access may be restricted, for security reasons, in your kernel config**_

Please make sure that you have libpci dependency before compiling.

On Debian-based distros this is covered by installing **pcilib-dev** package:

    sudo apt install build-essential cmake libpci-dev

On Fedora:

    sudo dnf install cmake gcc-c++ pciutils-devel

On Arch:

    sudo pacman -S base-devel pciutils cmake


On OpenSUSE Tumbleweed:

    sudo zypper in cmake gcc14-c++ pciutils-devel

You may need to add the `iomem=relaxed` param to your kernel params on Tumbleweed, or [you may run into errors at runtime](https://github.com/FlyGoat/RyzenAdj/issues/241).

If your Distribution is not supported, try finding the packages or use [Distrobox](https://github.com/89luca89/distrobox) or [Toolbox](https://docs.fedoraproject.org/en-US/fedora-silverblue/toolbox/) instead.

The simplest way to build it:

    git clone https://github.com/FlyGoat/RyzenAdj.git
    cd RyzenAdj
    rm -r win32
    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
    if [ -d ~/.local/bin ]; then ln -s $(readlink -f ryzenadj) ~/.local/bin/ryzenadj && echo "symlinked to ~/.local/bin/ryzenadj"; fi
    if [ -d ~/.bin ]; then ln -s $(readlink -f ryzenadj) ~/.bin/ryzenadj && echo "symlinked to ~/.bin/ryzenadj"; fi

#### Ryzen\_smu

To let RyzenAdj use ryzen\_smu module, you have to install it first, it is not part of the linux kernel.

On Fedora:

```sh
sudo dnf install cmake gcc gcc-c++ dkms openssl
```

Clone and install ryzen\_smu:

```sh
git clone https://github.com/amkillam/ryzen_smu # Active fork of the original module
(cd ryzen_smu/ && sudo make dkms-install)
```

If you are using secure boot, you have to enroll the UEFI keys which dkms has generated on its first
run. These have to be added to your machines UEFI key database. This can be done with following
command, which will ask you to set a password. This password is only needed _one single time_ later
in the MOK manager.

```sh
sudo mokutil --import /var/lib/dkms/mok.pub
```

Restart your system. This will boot into the MOK manager. Choose `Enroll MOK`, enter your password
and then reboot.
[Here](https://github.com/dell/dkms/blob/f7f526c145ecc01fb4ac4eab3009b1879b14ced4/README.md#secure-boot)
are some screenshots describing the process.

The module is now loaded and visible via dmesg. It will show a message about the kernel being
tainted, but this just means it loaded a (potentially proprietary) binary blob.

Build and install RyzenAdj:

```sh
git clone https://github.com/FlyGoat/RyzenAdj
cd RyzenAdj
cmake -B build -DCMAKE_BUILD_TYPE=Release
make -C build -j"$(nproc)"
sudo cp -v build/ryzenadj /usr/local/bin/
```

### Windows

It can be built by Visual Studio + MSVC automatically, or Clang + Nmake in command line.
However, as for now, MingW-gcc can't be used to compile for some reason.

Required dll is included in ./win32 of source tree. Please put the dll
library and sys driver in the same folder with ryzenadj.exe.

We don't recommend you to build by yourself on Windows since the environment configuration
is very complicated. If you would like to use ryzenadj functions in your program, see libryzenadj.
