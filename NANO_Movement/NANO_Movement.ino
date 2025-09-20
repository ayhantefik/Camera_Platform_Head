#include <Arduino_BMI270_BMM150.h> // IMU Sensor Library for Arduino Nano 33 BLE Rev.2
#include <ArduTFLite.h>
#include <ArduinoBLE.h>
#include "model.h"

const float accelerationThreshold = 1.55; // Threshold (in G values) to detect a "gesture" start
const int numSamples = 119; // Number of samples for a single gesture

int samplesRead; // sample counter 
const int inputLength = 714; // dimension of input tensor (6 values * 119 samples)

// The Tensor Arena memory area is used by TensorFlow Lite to store input, output and intermediate tensors
// It must be defined as a global array of byte (or u_int8 which is the same type on Arduino) 
// The Tensor Arena size must be defined by trials and errors. We use here a quite large value.
// The alignas(16) directive is used to ensure that the array is aligned on a 16-byte boundary,
// this is important for performance and to prevent some issues on ARM microcontroller architectures.
constexpr int tensorArenaSize = 8 * 1024;
alignas(16) byte tensorArena[tensorArenaSize];

// a simple table to map gesture labels
const char* GESTURES[] = {
  "left", // 1
  "right" // 2
};

#define NUM_GESTURES (sizeof(GESTURES) / sizeof(GESTURES[0]))

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // init IMU sensor
  if (!IMU.begin()) {
    Serial.println("IMU sensor init failed!");
    while (true); // stop program here.
  }

  // initialize the Bluetooth® Low Energy hardware
  BLE.begin();

  Serial.println("Bluetooth® Low Energy Central - GESTURE control");

  // start scanning for peripherals
  BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");

  // print IMU sampling frequencies
  Serial.print("Accelerometer sampling frequency = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.print("Gyroscope sampling frequency = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");

  Serial.println();
  Serial.println("Init model..");
  if (!modelInit(model, tensorArena, tensorArenaSize)){
    Serial.println("Model initialization failed!");
    while(true);
  }
  Serial.println("Model initialization done.");
}

void loop() {
  // check if a peripheral has been discovered
  BLEDevice peripheral = BLE.available();

  if (peripheral) {
    // discovered a peripheral, print out address, local name, and advertised service
    Serial.print("Found ");
    Serial.print(peripheral.address());
    Serial.print(" '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();

    if (peripheral.localName() != "GESTURE") {
      return;
    }
    // stop scanning
    BLE.stopScan();

    controlLed(peripheral);

    // peripheral disconnected, start scanning again
    BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
  }
}
void controlLed(BLEDevice peripheral) {
  float aX, aY, aZ, gX, gY, gZ;
  // connect to the peripheral
  Serial.println("Connecting ...");

  if (peripheral.connect()) {
    Serial.println("Connected");
  } else {
    Serial.println("Failed to connect!");
    return;
  }

  // discover peripheral attributes
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }

  // retrieve the GESTURE characteristic
  BLECharacteristic gestureCharacteristic = peripheral.characteristic("19b10000-e8f2-537e-4f6c-d104768a1214");

  if (!gestureCharacteristic) {
    Serial.println("Peripheral does not have characteristic!");
    peripheral.disconnect();
    return;
  } else if (!gestureCharacteristic.canWrite()) {
    Serial.println("Peripheral does not have a writable characteristic!");
    peripheral.disconnect();
    return;
  }

  while (peripheral.connected()) {
    // while the peripheral is connected
    // wait for a significant movement
    Serial.println("peripheral.connected");
    while (true) {
      if (IMU.accelerationAvailable()) {
        // read linear acceleration
        IMU.readAcceleration(aX, aY, aZ);

        // compute absolute value of total acceleration
        float aSum = fabs(aX) + fabs(aY) + fabs(aZ);

        // if total absolute acceleration is over the threshold a gesture has started
        if (aSum >= accelerationThreshold) {
          samplesRead = 0; // init samples counter
          break; // exit from waiting cycle
        }
      }
    }

    // reading cycle of all samples for current gesture
    while (samplesRead < numSamples) {
      // check if a sample is available
      if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
        // read acceleration and gyroscope values
        IMU.readAcceleration(aX, aY, aZ);
        IMU.readGyroscope(gX, gY, gZ);

        // normalize sensor data because model was trained using normalized data
        aX = (aX + 4.0) / 8.0;
        aY = (aY + 4.0) / 8.0;
        aZ = (aZ + 4.0) / 8.0;
        gX = (gX + 2000.0) / 4000.0;
        gY = (gY + 2000.0) / 4000.0;
        gZ = (gZ + 2000.0) / 4000.0;
        
        // put the 6 values of current sample in the proper position
        // in the input tensor of the model
        modelSetInput(aX,samplesRead * 6 + 0);
        modelSetInput(aY,samplesRead * 6 + 1);
        modelSetInput(aZ,samplesRead * 6 + 2); 
        modelSetInput(gX,samplesRead * 6 + 3);
        modelSetInput(gY,samplesRead * 6 + 4);
        modelSetInput(gZ,samplesRead * 6 + 5); 
        
        samplesRead++;
        
        if (samplesRead == numSamples) {
          if (!modelRunInference()) {
            Serial.println("RunInference Failed!");
            return;
          }

          // ----- find highest-probability gesture -----
          float maxProb   = 0.0f;
          int   maxIndex  = -1;

          for (int i = 0; i < NUM_GESTURES; ++i) {
            float p = modelGetOutput(i);     // probability in [0,1]
            if (p > maxProb) {               // keep the best so far
              maxProb  = p;
              maxIndex = i;
            }
          }

          // ----- print only the best gesture -----
          if (maxIndex >= 0) {               // safety check
            Serial.println(GESTURES[maxIndex]);
            int gestureNumber = getGestureNumber(GESTURES[maxIndex]);
            gestureCharacteristic.writeValue((byte)gestureNumber);
          }
        }
      }
    }
  }
}
int getGestureNumber(const char* gesture) {
  int numGestures = sizeof(GESTURES) / sizeof(GESTURES[0]);
  for (int i = 0; i < numGestures; ++i) {
    if (strcmp(GESTURES[i], gesture) == 0) {
      return i + 1;
    }
  }
  return -1; // Not found
}