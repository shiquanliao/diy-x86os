# 适用于mac
qemu-system-i386  -m 128M -s -S  -drive file=disk1.dmg,index=0,media=disk,format=raw -drive file=disk2.dmg,index=2,media=disk,format=raw -d int,page,mmu,cpu_reset,guest_errors,page,trace:ps2_keyboard_set_translation
