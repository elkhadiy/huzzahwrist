#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include "MPU6500.h"
#include "MPU6500RM.h"

#include "FS.h"
#include "SD.h"

#include "BluetoothSerial.h"

#include "esp_task_wdt.h"

TaskHandle_t sensorSample;
void sensorSampleTask(void *args);
TaskHandle_t write2SD;
void write2SDTask(void *args);
TaskHandle_t display2oled;
void display2oledTask(void *args);
QueueHandle_t q;

SPIClass *vspi;
SPIClass *hspi;
MPU6500 *mpu;

File file;

BluetoothSerial SerialBT;

void setup() {

  Serial.begin(2000000);

  SerialBT.begin("ELKSENS_1");

//  Serial.println("Waiting for start byte over Bluetooth");

  q = xQueueCreate(1000, 12);

  vspi = new SPIClass(VSPI);
  hspi = new SPIClass(HSPI);
  // SCLK = 33, MISO = 32, MOSI = 15, SS = 14
  hspi->begin(33, 32, 15, 14);

  SD.begin(21, *vspi, SPI_DATA_FREQ);

  SD.remove("/sensorData/accel.cap");
  file = SD.open("/sensorData/accel.cap", FILE_WRITE);

  mpu = new MPU6500();
  mpu->setup(MPU6500::SPI, hspi);
  mpu->calibrate();

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

  xTaskCreatePinnedToCore(
    display2oledTask,
    "Display2OLED",
    8192,
    NULL,
    1,
    &display2oled,
    1
  );

  esp_task_wdt_delete(sensorSample);
  esp_task_wdt_delete(write2SD);
  esp_task_wdt_delete(display2oled);

}

void loop() {
  vTaskDelete(NULL); // remove loopTask
}

void sensorSampleTask(void *args) {

  uint8_t values[12];

  for (;;) {
    mpu->rawspiSample(values);
    xQueueSend(q, (void *)values, 0);
    ets_delay_us(SAMPLING_DELAY);
  }

}

void write2SDTask(void *args) {

  // should probably have a synchronised start_recording based on rtc interrupt
  // test synchronisation with highspeed camera and led toggling

  uint8_t *bulk_vals = (uint8_t*) malloc(12 * 1000);

  for (;;) {

    int nb_msgs = uxQueueMessagesWaiting(q);

//    Serial.println(nb_msgs);

    for (int k = 0; k < nb_msgs; k++) {
      xQueueReceive(q, bulk_vals + 12 * k, 0);
    }

//    file.write(bulk_vals, 12 * nb_msgs);
//    file.flush();
      SerialBT.write(bulk_vals, 12 * nb_msgs);
//    Serial.write(bulk_vals, 12 * nb_msgs);
//    SerialBT.printf("%f %f %f\n",
//      (int16_t)(bulk_vals[0] << 8 | bulk_vals[1]) / 16384.0,
//      (int16_t)(bulk_vals[2] << 8 | bulk_vals[3]) / 16384.0,
//      (int16_t)(bulk_vals[4] << 8 | bulk_vals[5]) / 16384.0
//      );

    vTaskDelay(1 / portTICK_PERIOD_MS);
  }

}

void sleepDisplay(Adafruit_SSD1306* display) {
  display->ssd1306_command(SSD1306_DISPLAYOFF);
}

void wakeDisplay(Adafruit_SSD1306* display) {
  display->ssd1306_command(SSD1306_DISPLAYON);
}

void display2oledTask(void *args) {

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  //  display.setRotation(2);

  display.display();
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  pinMode(35, INPUT);

  for (;;) {
    display.clearDisplay();

    display.setTextColor(WHITE);
    display.setCursor(10, 0);
    display.print(F("Battery lvl: "));
    display.println(((double)analogRead(35) / 4095.0) * 2 * 3.3 * 1.1);
    display.print(F("Queue health: "));
    display.println(uxQueueMessagesWaiting(q));

    display.display();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }

}
