#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <TeensyThreads.h>

#define NUM_SENSORS 8
#define SD_CS BUILTIN_SDCARD


const int sensorPins[NUM_SENSORS] = {
  A0, A1, A2, A3, A4, A5, A6, A7 //pins type shi
};

//same shi as rtos
typedef struct {
  uint32_t timestamp;
  float pressure[NUM_SENSORS];
} Sample;

//same shi as rtos but says buffer size here rather than queue
#define BUFFER_SIZE 100

Sample buffer[BUFFER_SIZE];
volatile int head = 0;
volatile int tail = 0;

Threads::Mutex bufferMutex;

File dataFile;

//sensor task, priority high
//reads sensors and puts data in buffer

void sensorThread() {

  Sample s;

  while (1) {

    s.timestamp = millis();

    for (int i = 0; i < NUM_SENSORS; i++) {
      int raw = analogRead(sensorPins[i]);
      s.pressure[i] = (raw / 1023.0f) * 2000.0f;
    }

    bufferMutex.lock(); // protect buffer access

    int next = (head + 1) % BUFFER_SIZE;
    if (next != tail) {           // ifffff buffer not full
      buffer[head] = s;
      head = next;
    }
    // otherwise it aint gon do nuffin

    bufferMutex.unlock(); // release lock

    threads.delay_ms(1); // ~1000Hz
  }
}


//sd task, priority medium
void sdThread() {

  Sample s;

  while (1) {

    bool gotData = false;

    bufferMutex.lock();

    if (tail != head) {
      s = buffer[tail];
      tail = (tail + 1) % BUFFER_SIZE;
      gotData = true;
    }

    bufferMutex.unlock();

    if (gotData) {

      dataFile.print(s.timestamp);

      for (int i = 0; i < NUM_SENSORS; i++) {
        dataFile.print(",");
        dataFile.print(s.pressure[i], 2);
      }

      dataFile.println();
      dataFile.flush();
    }
    else {
      threads.yield();
    }
  }
}

//lora task, priority low

void loraThread() {

  while (1) {
    // again i aint doing ts, just look at telemetry's shi
    threads.delay_ms(100);
  }
}





void setup() {

  Serial.begin(115200);
  delay(1000);

  if (!SD.begin(SD_CS)) { //check sd
    Serial.println("SD failed");
    while (1);
  }

  dataFile = SD.open("data.csv", FILE_WRITE); //open file, dont remeber if i closed it or not
                                            // not my problem tho

  dataFile.print("time_ms");
  for (int i = 0; i < NUM_SENSORS; i++) { //again the num of sensors and how you deal with em is not
                                            // my problem
    dataFile.print(",p");
    dataFile.print(i);
  }
  dataFile.println();
  dataFile.flush();

  // priorities: higher = more important.
  //the big numbers are stack sizes, i just guessed them theyre prolly fine
  threads.addThread(sensorThread, 0, 2048, 3);
  threads.addThread(sdThread,     0, 4096, 2);
  threads.addThread(loraThread,   0, 2048, 1);
}

void loop() {
  // nothing necessary here idk
}
