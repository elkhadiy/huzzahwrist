#include "MPU6500.h"
#include "MPU6500RM.h"

#include "FS.h"
#include "SD.h"

#include "esp_task_wdt.h"

TaskHandle_t sensorSample;
void sensorSampleTask(void *args);
TaskHandle_t write2SD;
void write2SDTask(void *args);
QueueHandle_t q;


SPIClass vspi(VSPI);
MPU6500 mpu(vspi);
File file;

void setup() {

  Serial.begin(115200);

  q = xQueueCreate(10000, 12);

  SPIClass hspi(HSPI);
  // SCLK = 14, MISO = 27, MOSI = 33, SS = 15
  hspi.begin(14, 27, 33, 15);
  SD.begin(15, hspi, SPI_DATA_FREQ);

  SD.remove("/sensorData/accel.cap");
  file = SD.open("/sensorData/accel.cap", FILE_WRITE);

  mpu.setup(MPU6500::SPI);
  mpu.calibrate();

  disableCore0WDT();
  disableCore1WDT();

  xTaskCreatePinnedToCore(
    sensorSampleTask, // task function
    "Sensor Sampling", // task name
    8192, // task stack size TODO: make this value relevant
    NULL, // task input parameter
    10, // task priority
    &sensorSample, // task handle
    0 // core on which the task will run
  );

  xTaskCreatePinnedToCore(
    write2SDTask,
    "Write to SD",
    8192,
    NULL,
    10,
    &write2SD,
    1
  );
  
  if (esp_task_wdt_delete(sensorSample) != ESP_OK) {
    Serial.println("Failed to remove sensorSample task from WDT");
  }
  if (esp_task_wdt_delete(write2SD) != ESP_OK) {
    Serial.println("Failed to remove write2SD task from WDT");
  }

}

void loop() {
  vTaskDelete(NULL); // remove loopTask
}

void sensorSampleTask(void *args) {

  uint8_t values[12];

  for (;;) {
    mpu.rawspiSample(values);
    xQueueSend(q, (void *)values, 0);
    ets_delay_us(SAMPLING_DELAY);
  }

}

void write2SDTask(void *args) {

  uint8_t values[12];
  int i = 40000;

  for (;;) {
    if ((i > 0) && (xQueueReceive(q, values, 0) == pdPASS)) {
      file.write(values, sizeof(values));
      file.flush();
      i--;
    } else if (i == 0) {
      file.close();
    }
  }

}
