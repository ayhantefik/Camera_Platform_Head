#include <Servo.h>
#include <ArduinoBLE.h>

BLEService gestureService("19b10000-e8f2-537e-4f6c-d104768a1214"); // Bluetooth® Low Energy LED Service
// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEByteCharacteristic switchCharacteristic("19b10000-e8f2-537e-4f6c-d104768a1214", BLERead | BLEWrite);

Servo myServo;  // create servo object

int moveValue = 0; // 1 = left, 2 = right

void setup() {
  Serial.begin(9600);
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }
  // set advertised local name and service UUID:
  BLE.setLocalName("GESTURE");
  BLE.setAdvertisedService(gestureService);

  // add the characteristic to the service
  gestureService.addCharacteristic(switchCharacteristic);

  // add service
  BLE.addService(gestureService);

  // set the initial value for the characteristic:
  switchCharacteristic.writeValue(0);

  // start advertising
  BLE.advertise();

  Serial.println("BLE GESTURE Peripheral");

  myServo.attach(9);  // attach to pin 9
}

void loop() {
  // listen for Bluetooth® Low Energy peripherals to connect:
  BLEDevice central = BLE.central();

  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    // while the central is still connected to peripheral:
    while (central.connected()) {
      if (switchCharacteristic.written()) {
        uint8_t newValue = switchCharacteristic.value();
        Serial.println(newValue);
        if(moveValue != newValue && moveValue == 0){
          switch (newValue) {
            case 1: moveLeft(newValue); break;
            case 2: moveRight(newValue); break;
          }
        }
        else if(moveValue != newValue && moveValue != 0){
          startPos(moveValue);
        }
      }
    }

    // when the central disconnects, print it out:
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
  }
}

void startPos(int value){
  // From Right
  if(value == 1){
    for (int angle = 180; angle >= 90; angle--) {
      myServo.write(angle);
      delay(15);
    }
  }
  // From Left
  else if(value == 2){
    for (int angle = 0; angle <= 90; angle++) {
      myServo.write(angle);
      delay(15);
    }
  }
  moveValue = 0;
}

void moveRight(int value){
  for (int angle = 90; angle >= 0; angle--) {
    myServo.write(angle);
    delay(15);
  }
  moveValue = value;
}
void moveLeft(int value){
  for (int angle = 90; angle <= 180; angle++) {
    myServo.write(angle);
    delay(15);
  }
  moveValue = value;
}
