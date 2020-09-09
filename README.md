# xm510-utils
Utilities to run on xm510 based IP cams

## Compile
* Download musl based toolchain from [OpenIPC](https://github.com/widgetii/ct-ng-builds/releases)
* put the full path of the `arm-unknown-linux-musleabi-gcc` binary in the GCC variable in the Makefile
* $ make [target]
  * (optionally pack [targets] using upx to safe some space)
* if you did not put the `ftp` binary on the cam yet (otherwise go to next step):
  * use [stokie-ant](https://github.com/stokie-ant/xm510_firmtool)'s scripts to pack the binary in a new firmware image
  * flash using [DeviceManager.exe](http://www.xiongmaitech.com/en/index.php/service/down_detail/83/187) (works via wine too)
* if the ftp binary from this repo is on the camera:
  * start an ftp server on the current directory: `$ twistd -n ftp -r.` (don't miss the dot at the end)
  * copy the desired binary to the camera: `xmhdipc@ipcam:# ftp <server ip> <server port> <filename>`
