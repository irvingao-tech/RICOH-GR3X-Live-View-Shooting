# GR IIIx + StickS3 快速开始

当前实机状态：GR IIIx 的 BLE 安全配对、自动重连、Wi-Fi Live View 和蓝色按键快门已经验证成功。GPS/BDS Unit v1.1 已按 StickS3 Grove G9/G10 接入，但 EXIF 写入仍处于实验阶段。

## 烧录

```powershell
pio run -e m5stack-sticks3
pio run -e m5stack-sticks3 --target upload --upload-port COM4
```

将 `COM4` 替换成 StickS3 的实际串口。若上传超时，关闭占用串口的软件，长按侧面按键直到绿色灯闪烁，再重试。

## 首次配对

1. 删除相机端以前失败的 StickS3 配对记录。
2. 在相机中选择新增设备/执行配对。
3. 复位 StickS3，等待 BLE 搜索与连接。
4. 相机显示六位验证码后，在 StickS3 PIN 页面输入：短按 Button A 让当前数字加一，短按 Button B 确认并移到下一位。无需 Windows 电脑。也可以选择提前运行：

```powershell
py -m pip install pyserial
py tools\pin_entry_gui.py COM4
```

5. 配对成功后等待 StickS3 自动显示实时取景。

## 拍照

实时取景出现后，短按 StickS3 正面的蓝色按键进行自动对焦和拍照。照片保存在相机 SD 卡。

短按 Button B 可停止 LiveView、关闭相机 Wi-Fi并返回相机本机操作模式；相机屏幕和转盘恢复可用。再次短按 Button B 可重新启动 StickS3 LiveView。长按 Button B 约 3 秒会清除 StickS3 内保存的 BLE 配对数据。

## GPS

GPS Unit v1.1 可通过 Grove 线连接 StickS3 底部 HY2.0-4P 接口。固件会打开 Grove 5V 输出，并默认用 `G9` 作为 GPS RX、`G10` 作为 GPS TX。不连接 GPS 不影响其他功能。
