# RyzenAdj
Adjust power management settings for Ryzen Processors.

Based on: [FlyGoat/ryzen_nb_smu](https://github.com/flygoat/ryzen_nb_smu)

Thanks a lot to "JustSkill" for his Amazing GUI interface!

Also, you can choose [le.storm1er/ryzen-controller](https://gitlab.com/le.storm1er/ryzen-controller) as a alternative GUI interface.

## Usage
The command line interface is identical on both Windows and Unix-Like OS.

You should run it with Administrator on Windows or root on Linux.

You can write a shell script or bat to do it automaticly.

```
$./ryzenadj -h
Usage: ryzenadj [options] [[--] args]
   or: ryzenadj [options]

 Ryzen Power Management adjust tool.

    -h, --help                        show this help message and exit

Options
    -i, --info                        Show information (W.I.P.)

Settings
    -a, --stapm-limit=<u32>           Sustained power limit (mW)
    -b, --fast-limit=<u32>            Fast PPT power limit (mW)
    -c, --slow-limit=<u32>            Slow PPT power limit (mW)
    -d, --slow-time=<u32>             Slow PPT constant time (S)
    -e, --stapm-time=<u32>            STAPM constant time (S)
    -f, --tctl-temp=<u32>             Tctl temperature (▒?
    -g, --vrm-current=<u32>           VRM Current Limit (mA)
    -j, --vrmsoc-current=<u32>        VRM SoC Current Limit (mA)
    -k, --vrmmax-current=<u32>        VRM Maximum Current Limit (mA)
    -l, --vrmsocmax-current=<u32>     VRM SoC Maximum Current Limit (mA)
    -m, --psi0-current=<u32>          PSI0 Current Limit (mA)
    -n, --psi0soc-current=<u32>       PSI0 SoC Current Limit (mA)
    -o, --max-socclk-frequency=<u32>  Maximum SoC Clock Frequency (MHz)
    -p, --min-socclk-frequency=<u32>  Minimum SoC Clock Frequency (MHz)
    -q, --max-fclk-frequency=<u32>    Maximum Transmission (CPU-GPU) Frequency (MHz)
    -r, --min-fclk-frequency=<u32>    Minimum Transmission (CPU-GPU) Frequency (MHz)
    -s, --max-vcn=<u32>               Maximum Video Core Next (VCE - Video Coding Engine) (Value)
    -t, --min-vcn=<u32>               Minimum Video Core Next (VCE - Video Coding Engine) (Value)
    -u, --max-lclk=<u32>              Maximum Data Launch Clock (Value)
    -v, --min-lclk=<u32>              Minimum Data Launch Clock (Value)

``` 

### demo
If I'm going to set all the Power Limit to 45W, and Tctl to 90 ℃,
then the command line should be:
```
./ryzenadj --stapm-limit=45000 --fast-limit=45000 --slow-limit=45000 --tctl-temp=90
```

## Build

### Build Requirements

Building this tool requires C & C++ compilers as well as **cmake**. It
requires privileged access to NB PCI config space, in order to compile it
one must have pcilib library & headers available.

### Linux

Please make sure that you have libpci dependency before compiling. On
Debian-based distros this is covered by installing **pcilib-dev** package:

    sudo apt install libpci-dev

The simplest way to build it:

    cmake CMakeLists.txt
    make

### Windows

It can be built by Visual Studio automaticly. However, as for now, MingW can't
be used to compile for some reason.

Required dll is included in ./prebuilt of source tree. Please put the dll
library and sys driver in the same folder with ryzenadj.exe.

## TODO
- Determine the unit of time
- Add more options
- Code cleanup (The original program was written in C but now it's C++ in
order to call VC++ library)

