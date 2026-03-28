IoT Smart Messaging Device (ESP32)

## Giới thiệu
MessageBox là một thiết bị IoT giúp kết nối các cặp đôi từ xa. Thiết bị cho phép gửi tin nhắn thời gian thực, nhắc nhở kỷ niệm và tích hợp công cụ hỗ trợ tập trung (Pomodoro).

Dự án được xây dựng trên nền tảng **ESP32** sử dụng kiến trúc **Dual-Core** để tối ưu hóa hiệu suất xử lý đa nhiệm (Multitasking).

## Key Features
* [cite_start]**Real-time Messaging:** Nhận tin nhắn từ Telegram Bot và hiển thị lên màn hình OLED ngay lập tức[cite: 5, 9, 21].
* **Dual-Core Processing:**
    * *Core 0:* Xử lý tác vụ mạng (WiFi, Telegram API, Blynk Cloud) để đảm bảo kết nối luôn ổn định.
    * [cite_start]*Core 1:* Xử lý giao diện (UI), cảm biến chạm và điều khiển động cơ Servo[cite: 35, 50].
* [cite_start]**Pomodoro Timer:** Tích hợp bộ đếm thời gian 25 phút giúp người dùng tập trung làm việc, đồng bộ trạng thái qua App Mobile[cite: 33].
* [cite_start]**Interactive Feedback:** Phản hồi xúc giác qua Servo và âm thanh (đang phát triển) khi có thông báo mới[cite: 26, 61].
* **Secure Design:** Thông tin nhạy cảm (WiFi, Token) được tách biệt khỏi Source Code (sử dụng `secrets.h`).

## Phần cứng (Hardware)
* [cite_start]**MCU:** ESP32 DevKit V1 (Dual Core, 240MHz)[cite: 53].
* [cite_start]**Display:** OLED 0.96 inch (I2C Driver SSD1306/SH1106)[cite: 54].
* [cite_start]**Actuator:** Servo SG90 (Feedback cơ học)[cite: 61].
* [cite_start]**Input:** Cảm biến chạm điện dung (Capacitive Touch Sensor)[cite: 46, 62].
* [cite_start]**Power:** Pin dự phòng với cơ chế "Keep-Alive" chống tự ngắt[cite: 49].

## Cài đặt & Sử dụng (Installation)
### 1. Chuẩn bị
* Cài đặt **Arduino IDE** và driver CP210x cho ESP32.
* Cài đặt các thư viện cần thiết: `Blynk`, `UniversalTelegramBot`, `U8g2`, `ESP32Servo`.

### 2. Cấu hình bảo mật
Để chạy được dự án, bạn cần tạo file `secrets.h` trong cùng thư mục với file `.ino` và điền thông tin của bạn:

```cpp
// secrets.h template
#define SECRET_WIFI_SSID     "YOUR_WIFI_NAME"
#define SECRET_WIFI_PASS     "YOUR_WIFI_PASS"
#define SECRET_BLYNK_AUTH_TOKEN "YOUR_BLYNK_TOKEN"
#define SECRET_BOT_TOKEN     "YOUR_TELEGRAM_BOT_TOKEN"
<<<<<<< HEAD
#define SECRET_CHAT_ID       "YOUR_CHAT_ID"
=======
#define SECRET_CHAT_ID       "YOUR_CHAT_ID"
>>>>>>> db11ecdb18875d79a74d5d959549a2ddda4ce4cf
