# RyzenAdj
Adjust power management settings for Ryzen Processors.

Based on: [FlyGoat/ryzen_nb_smu](https://github.com/flygoat/ryzen_nb_smu)

## Usage
The command line interface is identical on both Windows and Unix-Like OS.

You should run it with Administrator on Windows or root on Linux.

You can write a shell script or bat to do it automaticly.

```
$./ryzenadj -h

Usage: ryzenadj [options] [[--] args]
   or: ryzenadj [options]

 Ryzen Power Management adjust tool.

    -h, --help                show this help message and exit

Options
    -i, --info                Show information (W.I.P.)

Settings
    -a, --stapm-limit=<u32>   Sustained power limit (10e-3 W)
    -b, --fast-limit=<u32>    Fast PPT power limit (10e-3 W)
    -c, --slow-limit=<u32>    Slow PPT power limit (10e-3 W)
    -d, --slow-time=<u32>     Slow PPT constant time
    -e, --stapm-time=<u32>    STAMP constant time
    -f, --tctl-temp=<u32>     Tctl temperature (℃)
``` 

### demo
If I'm going to set all the Power Limit to 45W, and Tctl to 90 ℃,
then the command line should be:
```
./ryzenadj --stapm-limit=45000 --fast-limit=45000 --slow-limit=45000 --tctl-temp=90
```

## Build
### Linux
This project is using cmake, you can search for guides online about how to compile a cmake based program.

Please ensure you have libpci depdency before compile.

### Windows
It can be built by Visual Studio automaticly. However, as for now, MingW can't be used to compile for some reason.

Required dll is included in ./prebuilt of source tree. Please put the dll library and sys driver in the same folder with ryzenadj.exe.

## TODO
- Determine the unit of time
- Add more options
- Code cleanup (The original program was written in C but now it's C++ in order to call VC++ library)

