# 适用于mac
# 解释：-s -S 用于gdb调试
# -s shorthand for -gdb tcp::1234, i.e. open a gdbserver on TCP port 1234
# -S do not start CPU at startup (you must type 'c' in the monitor). 
qemu-system-i386 -m 128M -s -S  -drive file=disk.img,index=0,media=disk,format=raw