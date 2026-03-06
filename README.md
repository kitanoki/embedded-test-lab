# RK3576 Codec Test Console (Qt6 Widgets)

## Features
- One-click remote encode/decode via SSH.
- Auto pre-kill startup business processes before each run:
  - `pkill -f '/etc/loop.sh'`
  - `killall -9 luajit`
- Channel selection: `/dev/video11`, `/dev/video12`, `/dev/video13`.
- Encode parameter controls:
  - Resolution
  - Codec: H264/H265
  - Rate Control: CBR/VBR
  - Bitrate: 2-40 Mbps
  - KeyFrame Interval (GOP): 2-60
- Stop by recorded PID.
- Status refresh every 1 second via SSH:
  - CPU temperature
  - CPU usage
  - Memory usage
- Real-time stdout/stderr display using remote `tail -F`.
- Local save of UI log text.
- Remote logs saved as: `/userdata/log/[CaseName]_yyyy-MM-dd.log`.

## Important runtime requirement
This tool uses local `ssh` command through `QProcess`.
- Recommended: passwordless SSH key login from PC to board root account.
- This version supports password field in UI:
  - If sshpass exists, it uses sshpass -p <password> ssh ....
  - If SSH Binary is plink, it uses plink -pw <password> ....
  - If neither is available, OpenSSH runs in batch mode and will fail fast with a warning instead of hanging.

## Build (Windows/Linux)

```bash
cmake -S . -B build
cmake --build build --config Release
```

Run:
- Windows: `build/Release/Rk3576CodecConsole.exe`
- Linux: `./build/Rk3576CodecConsole`

## Notes
- Default decode command follows your demo (`DolbyVision_NASA_4K.mp4`).
- Default encode pipeline is gstreamer-based with RK encoder plugins (`mpph264enc` / `mpph265enc`).
- If plugin names differ in your SDK image, only `buildEncodeGstCommand()` in `MainWindow.cpp` needs adjustment.

