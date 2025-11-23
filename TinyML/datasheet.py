import pandas as pd
import numpy as np
from scipy.special import expit as sigmoid

np.random.seed(42)
n = 200  # số mẫu

# 1) Sinh T, RH theo nhiều vùng môi trường
counts = [
    int(n * 0.35),  # vùng bình thường / mát
    int(n * 0.30),  # nóng + ẩm (nguy cơ cao)
    int(n * 0.20),  # rất nóng nhưng ẩm vừa
    n               # phần còn lại
]
counts[3] = n - sum(counts[:3])

# 1. Bình thường / mát
temp_1 = np.random.uniform(22, 30, counts[0])
humi_1 = np.random.uniform(40, 65, counts[0])

# 2. Nóng + ẩm (điển hình nguy hiểm)
temp_2 = np.random.uniform(30, 37, counts[1])
humi_2 = np.random.uniform(70, 90, counts[1])

# 3. Rất nóng nhưng ẩm vừa
temp_3 = np.random.uniform(35, 42, counts[2])
humi_3 = np.random.uniform(40, 60, counts[2])

# 4. Hỗn hợp random
temp_4 = np.random.uniform(24, 36, counts[3])
humi_4 = np.random.uniform(50, 85, counts[3])

temperature = np.concatenate([temp_1, temp_2, temp_3, temp_4])
humidity    = np.concatenate([humi_1, humi_2, humi_3, humi_4])

# 2) Thêm noise + drift
temp_noise = np.random.normal(0, 0.3, n)
humi_noise = np.random.normal(0, 2.0, n)
drift      = np.linspace(0, 0.5, n)  # nhiệt độ drift nhẹ theo thời gian

temperature = temperature + temp_noise + drift
humidity    = humidity + humi_noise

# Giới hạn lại
temperature = np.clip(temperature, 15, 45)
humidity    = np.clip(humidity, 20, 100)

# 3) Hàm tính Heat Index (°C)
#    công thức chuẩn NOAA (Rothfusz regression)
def heat_index_celsius(T_c, RH):
    """
    T_c: nhiệt độ (°C)
    RH: độ ẩm (%)
    return: Heat Index (°C)
    """
    # đổi sang °F
    T = T_c * 9/5 + 32

    # công thức HI (đơn vị °F)
    HI = (
        -42.379
        + 2.04901523 * T
        + 10.14333127 * RH
        - 0.22475541 * T * RH
        - 6.83783e-3 * T**2
        - 5.481717e-2 * RH**2
        + 1.22874e-3 * T**2 * RH
        + 8.5282e-4 * T * RH**2
        - 1.99e-6 * T**2 * RH**2
    )

    mask_low = (RH < 13) & (T >= 80) & (T <= 112)
    HI[mask_low] -= ((13 - RH[mask_low]) / 4) * np.sqrt((17 - np.abs(T[mask_low] - 95)) / 17)

    mask_high = (RH > 85) & (T >= 80) & (T <= 87)
    HI[mask_high] += ((RH[mask_high] - 85) / 10) * ((87 - T[mask_high]) / 5)

    # đổi lại sang °C
    HI_c = (HI - 32) * 5/9
    return HI_c

# 4) Tính Heat Index + xác suất nguy cơ
HI = heat_index_celsius(temperature, humidity)

# ~ 27°C: bắt đầu khó chịu
# ~ 45°C: cực nguy hiểm
HI_norm = (HI - 27) / (45 - 27)
HI_norm = np.clip(HI_norm, 0, 1)

# Dùng sigmoid để ra xác suất label
# HI_norm = 0.5 -> xác suất ~0.5
# HI_norm cao hơn -> xác suất gần 1
prob = sigmoid((HI_norm - 0.5) * 7)

labels = (np.random.rand(n) < prob).astype(int)

# 5) Tạo DataFrame + lưu CSV
df = pd.DataFrame({
    "Temperature": np.round(temperature, 2),
    "Humidity":    np.round(humidity, 2),
    "Label":       labels,
})

df.to_csv("sensor_data.csv", index=False)

print("File sensor_data.csv đã được tạo!")
print(df.head(10))
