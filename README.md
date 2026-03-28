# LoveBox - IoT Smart Messaging Device

**LoveBox** là một chiếc hộp thông minh IoT được thiết kế đặc biệt dành cho các cặp đôi yêu xa. Không chỉ là một thiết bị nhận tin nhắn thời gian thực, LoveBox còn đóng vai trò như một người bạn đồng hành với các tính năng đếm ngày kỷ niệm, nhắc nhở công việc và phát nhạc tự động.

Dự án được phát triển trên vi điều khiển **ESP32**, tận dụng tối đa kiến trúc Dual-Core để vừa duy trì kết nối mạng ổn định, vừa đảm bảo hiệu ứng giao diện (UI) và âm thanh mượt mà.

---

##  Tính năng nổi bật

### 1. Kết nối & Tương tác thông minh
* **Nhận tin nhắn Real-time:** Tích hợp Telegram Bot và Blynk, hiển thị tin nhắn ngay lập tức lên màn hình OLED.
* **Cảm ứng điện dung siêu nhạy:** Sử dụng phím chạm kim loại với thuật toán lọc nhiễu tự động để đọc lệnh điều khiển (Chạm đơn, Chạm đúp, Chạm giữ).
* **Phản hồi xúc giác (Haptic Feedback):** Cơ cấu Servo SG90 gõ vào vỏ hộp tạo âm thanh vật lý báo hiệu khi có thông báo mới.

### 2. Tiện ích & Giải trí
* **Music Player:** Tích hợp module MP3 DFPlayer Mini, hỗ trợ phát danh sách 100 bài hát với chế độ xáo trộn (Shuffle) cực kỳ mượt mà.
* **Pomodoro Timer:** Bộ đếm thời gian tập trung 25 phút, tự động tắt nhạc để làm việc và đồng bộ trạng thái qua App.
* **Đồng hồ & Báo thức:** Tự động đồng bộ thời gian thực qua NTP (Google Time), hẹn giờ báo thức từ xa.
* **To-do List & Kỷ niệm:** Nhắc nhở công việc qua ghi chú và đếm số ngày yêu nhau tự động.

### 3. Hệ thống cốt lõi (Core System)
* **Dual-Core Multitasking:** Task xử lý mạng (WiFi, Telegram) chạy độc lập trên Core 0, giao diện và phần cứng chạy trên Core 1.
* **Safe Mode & Watchdog (WDT):** Tự động phát hiện lỗi tràn bộ nhớ hoặc mất kết nối, tự động khởi động lại và vào chế độ an toàn nếu crash liên tục 3 lần.
* **OTA Update:** Hỗ trợ nạp code từ xa qua mạng WiFi không cần cắm cáp.

---

## 🛠 Phần cứng yêu cầu (Hardware)
* **MCU:** ESP32 DevKit V1
* **Màn hình:** OLED 0.96" (SH1106 - Giao tiếp I2C)
* **Âm thanh:** DFRobot DFPlayer Mini + Loa 3W
* **Động cơ:** Servo SG90
* **Cảm biến:** Cảm biến chạm điện dung (Nối trực tiếp qua chân Touch của ESP32)

### Sơ đồ nối chân (Pinout)
| Linh kiện | Chân trên ESP32 | Ghi chú |
| :--- | :--- | :--- |
| **OLED (SDA/SCL)** | D21 / D22 | Giao tiếp I2C tiêu chuẩn |
| **DFPlayer (RX/TX)** | D33 / D32 | Serial 2 (Hardware Serial) |
| **Servo SG90** | D15 | Điều khiển PWM |
| **Touch Sensor** | D4 | Chân cảm biến điện dung |

---

## Hướng dẫn cài đặt (Installation)

### Bước 1: Chuẩn bị thư viện (Arduino IDE)
* `Blynk` (giao tiếp IoT)
* `UniversalTelegramBot` (nhận tin nhắn bot)
* `U8g2` (điều khiển màn hình OLED)
* `ESP32Servo` (điều khiển động cơ)
* `DFRobotDFPlayerMini` (điều khiển module MP3)

### Bước 2: Cấu hình bảo mật (Mật khẩu & Token)
Tạo một file tên là `secrets.h` đặt cùng thư mục với file `.ino`. Điền các thông tin cá nhân của bạn vào file này:
```cpp
#ifndef SECRETS_H
#define SECRETS_H
#pragma once

// 1. WiFi
#define WIFI_SSID     "Tên_WiFi_Của_Bạn"
#define WIFI_PASSWORD "Mật_khẩu_WiFi"

// 2. Blynk (Lấy từ Web Dashboard)
#define SECRET_BLYNK_TEMPLATE_ID    "TMPL_XXX"
#define SECRET_BLYNK_TEMPLATE_NAME  "LoveBox"
#define SECRET_BLYNK_AUTH_TOKEN     "Token_Blynk_Của_Bạn"

// 3. Telegram
#define SECRET_BOT_TOKEN     "Token_BotFather"
#define SECRET_CHAT_ID       "ID_Chat_Của_Bạn"

#endif
