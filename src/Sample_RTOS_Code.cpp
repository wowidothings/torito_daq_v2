#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#define NUM_SENSORS 8 //I HAVE NO CLUE HOW MANY SENSORS YOU HAVE, CHANGE AS NEEDED
#define SD_CS BUILTIN_SDCARD

const int sensorPins[NUM_SENSORS] = {
  A0, A1, A2, A3, A4, A5, A6, A7 // Figure out which pins you have to use
};

// What goes in the csv file, its time, p1, p2, ..., p8
typedef struct {
  uint32_t timestamp;
  float pressureSensors[NUM_SENSORS];
} Sample;

//QueueHandle_t alredy exists in rtos, this is making the pointer to it
QueueHandle_t dataQueue;

File SDCard;

//sensor, priority high

void SensorTask(void *pvParameters) {

  Sample s;

  while (1) {

    s.timestamp = millis(); //miliseconds is prolly enough

    for (int i = 0; i < NUM_SENSORS; i++) {
      int raw = analogRead(sensorPins[i]); // again i don't know the sensor implementation, i
                                            //I think you have a class youre pulling from
      s.pressure[i] = map(raw, 0, 1023, 0, 2000); //remove this if you have a class
    }

    // Send to queue (block if full)
    xQueueSend(dataQueue, &s, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(1)); // 1ms sample rate (like a 1000 Hz)
  }
}

//sd task, priority medium

void SDTask(void *pvParameters) {

  Sample s;

  while (1) {
//formatting csv
    if (xQueueReceive(dataQueue, &s, portMAX_DELAY) == pdPASS) { //wait till you get data, no polls

      dataFile.print(s.timestamp);

      for (int i = 0; i < NUM_SENSORS; i++) {
        dataFile.print(",");
        dataFile.print(s.pressure[i], 2);
      }

      dataFile.println();
      dataFile.flush();
    }
  }
}

//lora task, priority low
//didnt feel like doing this
void LoRaTask(void *pvParameters) {

  while (1) {
    // idfk
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}





void setup() {

  Serial.begin(115200);
  delay(1000);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD failed");
    while (1);
  }

  dataFile = SD.open("data.csv", FILE_WRITE);

  dataFile.print("time_ms");
  for (int i = 0; i < NUM_SENSORS; i++) {
    dataFile.print(",p");
    dataFile.print(i);
  }
  dataFile.println();
  dataFile.flush();

  // Create Queue (holds 100 samples)
  dataQueue = xQueueCreate(100, sizeof(Sample));

  if (dataQueue == NULL) {
    Serial.println("Queue creation failed");
    while (1);
  }

  // Create Tasks
  xTaskCreate(SensorTask, "Sensors", 2048, NULL, 3, NULL);
  xTaskCreate(SDTask,     "SD",      4096, NULL, 2, NULL);
  xTaskCreate(LoRaTask,   "LoRa",     2048, NULL, 1, NULL);

  vTaskStartScheduler();
}

void loop() {
  // Never runs under RTOS
}
