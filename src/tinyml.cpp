#include "tinyml.h"

// === Biến toàn cục cho TinyML ===
namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;

    constexpr int kTensorArenaSize = 8 * 1024;
    uint8_t tensor_arena[kTensorArenaSize];
}

// === Khởi tạo TinyML ===
void setupTinyML()
{
    Serial.println("TensorFlow Lite Init...");

    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(TinyML_model);
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        error_reporter->Report("Model version mismatch!");
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk)
    {
        error_reporter->Report("AllocateTensors() failed");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("TinyML model loaded successfully!");
}

// === Task chạy liên tục inference ===
void tiny_ml_task(void *pvParameters)
{
    setupTinyML();

    while (1)
    {
        // --- Chuẩn hóa dữ liệu đầu vào ---
        float norm_temp = glob_temperature / 40.0f;
        float norm_humi = glob_humidity / 100.0f;
        float norm_light = glob_light / 4095.0f;
        // --- Gán vào tensor đầu vào ---
        input->data.f[0] = norm_temp;   // Nhiệt độ
        input->data.f[1] = norm_humi;   // Độ ẩm
        input->data.f[2] = norm_light;  // Ánh sáng

        // --- Chạy mô hình ---
        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk)
        {
            error_reporter->Report("Invoke failed");
        }
        else
        {
            last_inference = output->data.f[0]; // Kết quả (0–1)

            // In ra log để kiểm tra
            Serial.printf(
                "[TinyML] Input (scaled): T=%.2f, H=%.2f, L=%.3f → Output=%.3f\n",
                norm_temp, norm_humi, norm_light, last_inference);


            if (last_inference > 0.5)
                Serial.println("AI dự đoán: Bất thường!");
            else
                Serial.println(" AI dự đoán: Bình thường.");
        }

        vTaskDelay(pdMS_TO_TICKS(3000)); // 3 giây / vòng
    }
}
