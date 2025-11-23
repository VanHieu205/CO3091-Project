import pandas as pd
import tensorflow as tf
from sklearn.model_selection import train_test_split
import numpy as np

# 1. Đọc dữ liệu
data = pd.read_csv("sensor_data.csv")
data = data.dropna()
cols = ["Temperature", "Humidity", "Label"]
for col in cols:
    data[col] = pd.to_numeric(data[col], errors='coerce')
data = data.dropna()

# 2. Tách X, y
X = data[["Temperature", "Humidity"]].values.astype(np.float32)
y = data["Label"].values.astype(np.float32)

# 3. Chuẩn hóa dữ liệu 0-1
X[:, 0] /= 40.0    # Temp 0–40°C
X[:, 1] /= 100.0   # Hum 0–100%

# 4. Train/test split
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# 5. Mô hình 2 input
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(2,)),
    tf.keras.layers.Dense(16, activation="relu"),
    tf.keras.layers.Dense(8, activation="relu"),
    tf.keras.layers.Dense(1, activation="sigmoid")
])

model.compile(optimizer="adam", loss="binary_crossentropy", metrics=["accuracy"])

# 6. Huấn luyện
model.fit(X_train, y_train, epochs=50, batch_size=16,
          validation_data=(X_test, y_test), verbose=1)

# 7. Đánh giá
loss, acc = model.evaluate(X_test, y_test)
print(f"Accuracy on test set: {acc*100:.2f}%")

# 8. Lưu model & TFLite
model.save("TinyML_model.h5")
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

with open("TinyML_model.tflite", "wb") as f:
    f.write(tflite_model)

# 9. Xuất header cho ESP32
with open("TinyML_model.tflite", "rb") as tflite_file:
    model_bytes = tflite_file.read()

c_array = ', '.join(f'0x{b:02x}' for b in model_bytes)
with open("TinyML_model.h", "w") as f:
    f.write("const unsigned char TinyML_model[] = {\n  " + c_array + "\n};\n")
    f.write(f"const int TinyML_model_len = {len(model_bytes)};\n")

print("Training & model export completed successfully!")
