cd /d %~dp0

adb push luahook /data/local/tmp/luahook
adb shell "su -c chmod -R 777 /data/local/tmp/luahook"
adb shell "su -c /data/local/tmp/injector com.CQUnreal.GShooter /data/local/tmp/libluahook.so"
adb shell "logcat -c"
adb shell "logcat|grep XMONO"