# Servo Dog Web Control 示例

基于 Web 的舵机狗控制示例，通过网页界面可以远程控制舵机狗的运动和进行舵机校准。

## 功能特性

- 网页控制界面，支持移动端访问
- 虚拟摇杆控制，支持前后左右移动
- 预置动作控制
- 舵机校准功能
- 支持 WiFi 连接和 SoftAP 模式
- 支持 mDNS，可通过域名访问

## 配置说明

在编译前，可以通过 `idf.py menuconfig` 进行以下配置：

### WiFi 设置

- WiFi SSID：要连接的 WiFi 网络名称
- WiFi Password：WiFi 密码
- SoftAP SSID：ESP32 创建的 WiFi 热点名称（默认为 "ESP-Hi"）
- SoftAP Password：热点密码
- SoftAP Channel：热点信道（默认为 1）
- SoftAP IP：热点 IP 地址（默认为 192.168.4.1）
- MDNS Hostname：mDNS 主机名（默认为 "esp-hi"，访问 `http://esp-hi.local/`）

## 使用说明

1. 编译并烧录程序到 ESP32
2. 根据配置的 WiFi 模式：
   - 如果配置了 WiFi 连接，ESP32 将连接到指定的 WiFi 网络
   - 如果配置了 SoftAP 模式，ESP32 将创建一个 WiFi 热点
3. 访问控制界面：
   - 如果使用 mDNS，访问 `http://[MDNS Hostname].local`
   - 如果使用 SoftAP，访问 `http://192.168.4.1`
   - 如果使用 WiFi 连接，访问 ESP32 的 IP 地址

### 控制界面

- **控制页面**：
  - 使用虚拟摇杆控制舵机狗移动
  - 使用预置动作按钮执行特定动作

- **舵机校准页面**：
  - 校准教程详见舵机校准页面提示

## 注意事项

1. 首次使用前需要先进行舵机校准
2. 不要徒手掰动舵机，以免损坏
