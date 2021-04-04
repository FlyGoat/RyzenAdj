import os, sys, time
from ctypes import *
from shutil import copyfile

lib_path = os.path.dirname(os.path.abspath(__file__))
os.chdir(lib_path)

if sys.platform == 'win32' or sys.platform == 'cygwin':
    try:
        os.add_dll_directory(lib_path)
    except AttributeError:
        pass #not needed for old python version

    winring0_driver_file_path = os.path.join(os.path.dirname(os.path.abspath(sys.executable)), 'WinRing0x64.sys')
    if not os.path.isfile(winring0_driver_file_path):
        copyfile(os.path.join(lib_path, 'WinRing0x64.sys'), winring0_driver_file_path)

    lib = cdll.LoadLibrary('libryzenadj')
else:
    lib = cdll.LoadLibrary('libryzenadj.so')

# define ctype mappings for types which can not be mapped automatically
lib.init_ryzenadj.restype = c_void_p
lib.refresh_table.argtypes = [c_void_p]
lib.get_fast_limit.restype = c_float
lib.get_fast_limit.argtypes = [c_void_p]

ry = lib.init_ryzenadj()

if not ry:
    sys.exit("RyzenAdj could not get initialized")

error_messages = {
    -1: "{:s} is not supported on this family\n",
    -3: "{:s} is not supported on this SMU\n",
    -4: "{:s} is rejected by SMU\n"
}

def adjust(field, value):
    function_name = "set_" + field
    adjust_func = lib.__getattr__(function_name)
    adjust_func.argtypes = [c_void_p, c_ulong]
    res = adjust_func(ry, value)
    if res:
        error = error_messages.get(res, "{:s} did fail with {:d}\n")
        sys.stderr.write(error.format(function_name, res));

def enable(field):
    function_name = "set_" + field
    adjust_func = lib.__getattr__(function_name)
    adjust_func.argtypes = [c_void_p]
    res = adjust_func(ry)
    if res:
        error = error_messages.get(res, "{:s} did fail with {:d}\n")
        sys.stderr.write(error.format(function_name, res));

print("Monitor if fast limit is not 35W")
while True:
    lib.refresh_table(ry)
    limit = round(lib.get_fast_limit(ry))
    if limit != 35:
        print("reapply limits, because old limit was {:d}".format(limit))
        adjust("fast_limit", 35000)
        adjust("slow_limit", 22000)
        adjust("slow_time", 30)
        adjust("tctl_temp", 97)
        adjust("apu_skin_temp_limit", 50)
        adjust("vrmmax_current", 100000)
        enable("max_performance")
    time.sleep(3)
