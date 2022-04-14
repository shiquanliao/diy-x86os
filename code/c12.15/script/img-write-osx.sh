# 写boot区，定位到磁盘开头，写1个块：512字节
dd if=boot.bin of=os.vhd bs=512 conv=notrunc count=1 

# 写loader区，定位到磁盘第2个块，写1个块：512字节
dd if=loader.bin of=os.vhd bs=512 conv=notrunc seek=1

# 写kernel区，定位到磁盘第100个块
dd if=kernel.elf of=os.vhd bs=512 conv=notrunc seek=100

# 写应用程序shell，临时使用
dd if=shell.elf of=os.vhd bs=512 conv=notrunc seek=1000
