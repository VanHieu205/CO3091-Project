# Giới thiệu
Trong bối cảnh Internet of Things (IoT) đang trở thành nền tảng quan trọng trong các hệ thống tự động hóa và giám sát thông minh, việc tích hợp cảm biến, xử lý dữ liệu và điều khiển thiết bị từ xa đóng vai trò then chốt trong nhiều ứng dụng thực tế như nông nghiệp, môi trường, nhà thông minh hay giám sát công nghiệp. Đồ án này được thực hiện nhằm xây dựng một hệ thống IoT hoàn chỉnh dựa trên vi điều khiển ESP32-S3, tích hợp nhiều thành phần liên quan từ thu thập dữ liệu cảm biến, truyền thông Internet, điều khiển thiết bị ngoại vi, đến xử lý AI và kết nối lên server đám mây.

Hệ thống có khả năng thu thập nhiệt độ, độ ẩm và cường độ ánh sáng theo thời gian thực, sau đó hiển thị dữ liệu trên LCD, trình bày trực quan qua webserver cũng như gửi lên nền tảng Core IoT để giám sát từ xa. Bên cạnh đó, đồ án xây dựng mô hình dự đoán mức độ an toàn môi trường dựa trên TinyML, so sánh hiệu suất giữa mô hình lý thuyết và mô hình triển khai trực tiếp trên phần cứng. Song song, hệ thống hỗ trợ điều khiển LED thường, LED Neopixel với nhiều chế độ hiển thị theo cảm biến hoặc điều khiển từ mạng.

Với việc đồng thời kết hợp FreeRTOS scheduling, Web API, MQTT, TinyML, và CoreIoT Cloud Integration, đồ án không chỉ giúp kiểm chứng hoạt động của hệ thống IoT thực tế mà còn cho phép mở rộng thành một nền tảng giám sát – điều khiển thông minh ứng dụng vào môi trường sống hoặc công nghiệp. Đồ án hướng tới mục tiêu tạo ra một hệ thống hoàn chỉnh, ổn định, trực quan và có tính ứng dụng cao.
# Sinh viên thực hiện
| STT | Họ và Tên           | MSSV     |
|-----|----------------------|----------|
| 1   | Nguyễn Văn Hiếu      | 2310967  |
| 2   | Trần Đình Hoàng      | 2311075  |
| 3   | Nguyễn Minh Hạnh     | 2310895  |
| 4   | Đàm Cao Minh Công    | 2310369  |
# Tính năng

1. Đọc nhiệt độ, độ ẩm realtime và hiển thị lên LCD.
2. Chớp tắt led blinky theo logic định nghĩa theo nhiệt độ, độ ẩm.
3. Điều khiển logic đèn Neopixel theo nhiệt độ, độ ẩm.
4. Hiện thực một webserver có thể điều khiển led đơn, Neopixel. Ngoài ra bao gồm các tính năng như hiển thị realtime biểu đồ nhiệt độ, độ ẩm, ánh sáng. Các tính năng điều chỉnh chế độ Wifi (AP, STA) của ESP32-S3, tích hợp dự đoán của AI dụa trên mô hình TinyML được áp dụng ở task5.
5. Triển khai mô hình TinyML, với dữ liệu mẫu được tạo sẵn. So sánh độ chính xác giữa mô hình lý thuyết và mô hình được nạp vào phần cứng.
6. Triển khai hệ thống lên Server CoreIoT, dùng Rule Chain để lấy dữ liệu từ một ESP32-S3 tới một ESP32-S3 khác.


# Cấu trúc thư mục
```
CO3091-Project/
├── .pio/                   # Files PlatformIO build
├── .vscode/                # VSCode configs
├── boards/                 # Board-specific config
├── include/                # Header files
│   ├── coreiot.h
│   ├── global.h
│   ├── led_blinky.h
│   ├── mainserver.h
│   ├── monitor_button.h
│   ├── neo_blinky.h
│   ├── temp_humi_monitor.h
│   └── tinyml.h
├── lib/
├── src/                    # Mã nguồn chính
│   ├── coreiot.cpp         # MQTT
│   ├── global.cpp          # Config chung
│   ├── led_blinky.cpp      # LED blink
│   ├── mainserver.cpp      # Main task
│   ├── mainserver.cpp      # Server handler
│   ├── monitor_button.cpp  # Button handler for Neo
│   ├── neo_blinky.cpp      # NeoPixel LED
│   ├── temp_humi_monitor.cpp # Đọc nhiệt độ, độ ẩm...
│   ├── TinyML_model.h      # TinyML model header
│   └── tinyml.cpp          # TinyML runtime
├── test/
|   ├── Publisher.py        #Test connect device
│   ├── TinyMQTTClientSubscriber.py           #Test subscriber
|   └── TinyMQTTClientPublisher.py      #Test alter to call rpc
├── TinyML/                 # Training & model building
│   ├── datasheet.py
│   ├── sensor_data.csv
│   ├── TinyML_model.h5
│   ├── TinyML_model.py
│   └── TinyML_model.tflite
└── .gitignore
```



# Cách build và chạy dự án
1. Cài PlatformIO:
Dùng VSCode Extension Marketplace: ```https://platformio.org/install```
2. Clone dự án
```git clone https://github.com/VanHieu205/CO3091-Project ```
3. Build
Vào PlatformIO Core CLI: ```pio run ```
4. Upload firmware vào board
Vào PlatformIO Core CLI: ```pio run -t upload ```
5. Mở Serial Monitor
Mở Serial Monitor và quan sát các kết quả được in ra.

## TinyML – Tạo và Training mô hình lý thuyết
1. Tạo tập datasheet :  ```python datasheet.py ```
2. Train model và chuyển model qua TFLite: ```python TinyML_model.py```
3. Nhúng vào firmware: Dùng script hoặc copy file .h đã generate vào project```

## So sánh mô hình lý thuyết và trên mạch
1. Comment tất cả các task ngoại trừ task ```evaluation_task ``` trong main.cpp. Tiến hành chạy nối mạch theo thiết kế, đóng tất cả các cổng Serial Montitor đang chạy sau đó chạy task này.
2. Chạy file python ```evaluate_on_hw.py ```. Lưu ý: Sử dụng đúng cổng kết nối được nối với phần cứng, trong demo, nhóm sử dụng cổng COM3: ```SERIAL_PORT = 'COM3' ```
3. Tiến hành quan sát và so sánh kết quả thu được với kết quả lý thuyết chạy ở hướng dẫn phía trên.

##**Kiểm thử các chức năng coreiot**
1. Chạy  ```python Publisher.py ```  để connect upload dữ liệu lên server Coreiot và hiện lên dashboard
2. Chạy  ```python TinyMQTTClientSubscriber.py ``` với token của device riêng biệt và chạy   ```python TinyMQTTClientPublisher.py ``` với token của device tên "alter to call rpc" & các giá trị long, lat như 
---
   HCMUT
long = 106.65789107082472
lat = 10.772175109674038

---
#H6
#long = 106.80633605864662
#lat = 10.880018410410052

---

nếu là HCMUT thì subcriber tại ESP32_002 nhận rpc liên quan tới power on or off ngược lại là ESP32_001


