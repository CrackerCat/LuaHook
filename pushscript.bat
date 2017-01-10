cd %~dp0
adb push luahook /data/local/tmp/luahook
adb shell "su -c chmod -R 777 /data/local/tmp/luahook"