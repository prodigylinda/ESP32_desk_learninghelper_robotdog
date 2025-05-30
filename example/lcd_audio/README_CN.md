# LCD 音频示例

本示例展示了 ESP32-C3 的高速图片刷新能力，并结合了音频播放功能。它展示了 ESP32-C3 如何同时处理显示和音频处理。

## 功能特点

- LCD 显示屏高速图片刷新
- 同步音频播放
- 支持多种图片格式
- 高效内存管理
- 实时显示和音频处理

## 硬件要求

- ESP-Hi 开发板

## 项目结构

```
lcd_audio/
├── main/           # 主应用程序代码
├── audio/          # 音频相关文件
├── gif/            # 图片资源
└── CMakeLists.txt  # 构建配置
```

## 构建和运行

1. 设置 ESP-IDF 环境
2. 构建项目：
   ```bash
   idf.py build
   ```
3. 烧录和监控：
   ```bash
   idf.py -p (PORT) flash monitor
   ```
