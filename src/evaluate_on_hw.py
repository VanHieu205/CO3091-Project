import serial
import time
import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split


try:
    data = pd.read_csv("sensor_data.csv")
except FileNotFoundError:
    print("Không tìm thấy file 'sensor_data.csv'.")
    exit()

data = data.dropna()
cols = ["Temperature", "Humidity", "Label"]
for col in cols:
    data[col] = pd.to_numeric(data[col], errors='coerce')
data = data.dropna()

X = data[["Temperature", "Humidity"]].values.astype(np.float32)
y = data["Label"].values.astype(np.float32)

_, X_test, _, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

print(f"Đã tải {len(X_test)} mẫu kiểm tra.")


SERIAL_PORT = 'COM3'
BAUD_RATE = 115200

try:
    esp32 = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
    print(f"Đã kết nối với ESP32 trên cổng {SERIAL_PORT}")
    ready_signal = esp32.readline().decode('utf-8').strip()
    if "Ready" not in ready_signal:
        print("Cảnh báo: Không nhận được tín hiệu 'Ready' từ ESP32.")
        print(f"Nhận được: '{ready_signal}'")
    else:
        print(f"ESP32 báo: {ready_signal}")
        
except Exception as e:
    print(f"LỖI: Không thể mở cổng {SERIAL_PORT}. {e}")
    exit()

correct_predictions = 0
total_samples = len(X_test)

print("Bắt đầu gửi dữ liệu ")

for i in range(total_samples):

    temp = X_test[i][0]
    humi = X_test[i][1]
    
    correct_label = int(y_test[i])

    data_to_send = f"{temp:.2f},{humi:.2f}\n"
    
    try:
        esp32.write(data_to_send.encode('utf-8'))
        
        prediction_str = esp32.readline().decode('utf-8').strip()
        
        if not prediction_str:
            print(f"Lỗi: ESP32 không trả lời cho mẫu {i+1}. Bỏ qua.")
            continue
            
        prediction_hw = int(prediction_str)
        
        if prediction_hw == correct_label:
            correct_predictions += 1
            status = "ĐÚNG"
        else:
            status = "SAI"
            
        print(f"Mẫu {i+1}/{total_samples}: Input=({temp:.1f}, {humi:.1f}) | "
              f"ESP32 dự đoán={prediction_hw} | "
              f"Đáp án đúng={correct_label} -> {status}")

    except Exception as e:
        print(f"Lỗi nghiêm trọng khi test mẫu {i+1}: {e}")
        break 

esp32.close()

if total_samples > 0:
    accuracy_on_hardware = (correct_predictions / total_samples) * 100
    print(f"Tổng số mẫu kiểm tra: {total_samples}")
    print(f"Số mẫu dự đoán đúng: {correct_predictions}")
    print(f"==> Accuracy trên ESP32: {accuracy_on_hardware:.2f}%")
else:
    print("Không có mẫu nào được kiểm tra.")