import os, sys, time
from ctypes import *
from shutil import copyfile

lib_path = os.path.dirname(os.path.abspath(__file__))
os.chdir(lib_path)

if sys.platform == 'win32':
    try:
        os.add_dll_directory(lib_path)
    except AttributeError:
        pass #not needed for old python version

    winring0_driver_file_path = os.path.join(os.path.dirname(os.path.abspath(sys.executable)), 'WinRing0x64.sys')
    if not os.path.isfile(winring0_driver_file_path):
        copyfile(os.path.join(lib_path, 'WinRing0x64.sys'), winring0_driver_file_path)

lib = cdll.LoadLibrary('libryzenadj')
# define ctype mappings for types which can not be mapped automatically
lib.init_ryzenadj.restype = c_void_p
lib.get_table_ver.argtypes = [c_void_p]
lib.get_table_size.argtypes = [c_void_p]
lib.get_new_table.argtypes = [c_void_p, c_void_p, c_size_t]
lib.get_fast_limit.restype = c_float
lib.get_fast_limit.argtypes = [c_void_p]

ry = lib.init_ryzenadj()

if ry == 0:
    sys.exit("RyzenAdj could not get initialized")

print("fast_limit: {:f}".format(lib.get_fast_limit(ry)))

print("ptable version: {:x}".format(lib.get_table_ver(ry)))

input("Press any key to show all ptable values...")

ptable_size = lib.get_table_size(ry)
ptable = (c_float * (ptable_size // 4))()

while True:
    lib.get_new_table(ry, ptable, ptable_size)
    columns, lines = os.get_terminal_size()
    table_columns = columns // 16 # 16 chars per table entry
    os.system('cls' if sys.platform == 'win32' else 'clear')
    table_rows = 0
    for index in range(len(ptable)):
        sys.stdout.write("{:3d}:{:8.2f}\t".format(index, ptable[index]))
        if index % table_columns == table_columns - 1: 
            sys.stdout.write('\n')
            table_rows += 1
            if table_rows >= lines - 1:
                sys.stdout.write('{:d} More entries ...'.format(len(ptable) - 1 - index))
                break
    
    if index % table_columns != table_columns - 1: sys.stdout.write('\n')
    sys.stdout.flush()
    time.sleep(1)