adb devices
adb shell mount -o remount,rw /dev/block/nandc /system
adb shell mount -o remount,rw /dev/root /
adb push disp/disp.ko /vendor/modules/disp.ko
adb shell chmod 600 /vendor/modules/disp.ko
adb push lcd/lcd.ko /vendor/modules/lcd.ko
adb shell chmod 600 /vendor/modules/lcd.ko
adb push hdmi/hdmi.ko /vendor/modules/hdmi.ko
adb shell chmod 600 /vendor/modules/hdmi.ko
adb shell sync
echo press any key to reboot
pause
adb shell reboot