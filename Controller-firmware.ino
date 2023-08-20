// Version 0.1.2 CreateMessageJoystickXY

#include <RH_ASK.h>
#include <SPI.h>

#define DEBUG_ENABLED

RH_ASK driver;

/* Transmit message creation variables */
const int ANALOG_SIZE_CONST{4};

const int JOYSTICK_XY_MSG_LENGTH{9};     // HXXXXYYYY
const int JOYSTICK_BUTTON_MSG_LENGTH{2}; // HV
const int BUTTON_MSG_LENGTH{2};          // HV

const char JOYSTICK_XY_MSG_HEADER{'0'};
const char JOYSTICK_BUTTON_MSG_HEADER{'1'};
const char BUTTON_MSG_HEADER{'2'};

char *jxy_msg{};
char *jbutt_msg{};
char *butt_msg{};

/* Pins */

/* RH transmitter */

const int RH_TransPin = 12;

/* Joystick pins */
const int joystickPinX = 0;    // A0
const int joystickPinY = 1;    // A1
const int joystickPinButt = 4; // D2

/* Button pins */
const int buttonPin = 7; // D7

/* Button variables */
int buttonState = 0;
int lastButtonState = 0;
int lastDebounceTime = 0;
int debounceDelay = 0;

/// @brief Helper function to convert int into int array: 1234 -> |1|2|3|4|
/// @param value
/// @return int* (array pointer)
int *Helper_ConvertIntToArr(int value) {
  int value_copy{value};
  int digits_in_the_number{1};

  while (value / 10 != 0) {
    value /= 10;
    digits_in_the_number++;
  }

  int *result_digits = new int[digits_in_the_number];

  for (int i = 0; i < digits_in_the_number; i++) {
    result_digits[i] = 0;
  }
  int digits_in_the_number_cp = digits_in_the_number;

  for (int i = 0; digits_in_the_number > 0; i++) {
#ifdef DEBUG_ENABLED
    Serial.print("digits_in_the_number:");
    Serial.println(digits_in_the_number);
#endif

    int pow10{1};
    for (int i = 0; i < digits_in_the_number - 1; i++) {
      pow10 *= 10;
    }

    result_digits[i] = value_copy / pow10;
#ifdef DEBUG_ENABLED
    Serial.print("[Helper_ConvertIntToArr] Result digits:");
    Serial.println(result_digits[i]);
#endif
    int mult_res = result_digits[i] * pow10;
    value_copy = value_copy - mult_res;

    Serial.print("value_copy after operation:");
    Serial.println(value_copy);

    digits_in_the_number--;

#ifdef DEBUG_ENABLED
    Serial.println("[Helper_ConvertIntToArr] Reslt digits:");
    for (int i = 0; i < digits_in_the_number_cp; i++) {
      Serial.print(result_digits[i]);
    }
    Serial.println();
#endif
  }

  return result_digits;
}

/// @brief Setup
void setup() {
  Serial.begin(9600);
  pinMode(RH_TransPin, OUTPUT);
  if (!driver.init()) {
    Serial.println("Initialization of RH driver failed");
  }

  pinMode(joystickPinX, INPUT);
  pinMode(joystickPinY, INPUT);
  pinMode(joystickPinButt, INPUT);
  pinMode(buttonPin, INPUT);

  jxy_msg = new char[JOYSTICK_XY_MSG_LENGTH];
  jbutt_msg = new char[JOYSTICK_BUTTON_MSG_LENGTH];
  butt_msg = new char[BUTTON_MSG_LENGTH];
}

/// @brief Get X value from joystick
/// @return int Joystick X (analog 0-1023)
int JoyStickGetX() {
#ifdef DEBUG_ENABLED
  Serial.println("Get data from the Joystick X");
#endif
  return analogRead(joystickPinX);
}

/// @brief Get Y value from joystick
/// @return int Joystick Y (analog 0-1023)
int JoyStickGetY() {
#ifdef DEBUG_ENABLED
  Serial.println("Get data from the Joystick Y");
#endif
  return analogRead(joystickPinY);
}

/// @brief Get button value from joystick
/// @return bool Joystick Button (digital 0-1)
bool JoyStcikIsButtonPressed() { return digitalRead(joystickPinButt); }

/// @brief Check if button is pressed
void ButtonIsPressed() {
  int currentButtonState = digitalRead(buttonPin);
  Serial.println(currentButtonState);

  if (currentButtonState != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currentButtonState != buttonState) {
      buttonState = currentButtonState;
    }
  }

  lastButtonState = currentButtonState;
}

/// @brief Create joystick-XY-info message for Radio transmit
/// @param x_value int
/// @param y_value int
void CreateMessageJoystickXY(int x_value, int y_value) {
#ifdef DEBUG_ENABLED
  Serial.println("[CreateMessageJoystickXY] CreateMessageJoystickXY");
  Serial.print("[CreateMessageJoystickXY] Received x: ");
  Serial.println(x_value);
  Serial.print("[CreateMessageJoystickXY] Received y: ");
  Serial.println(y_value);
#endif
  jxy_msg[0] = JOYSTICK_XY_MSG_HEADER;

  int *x_array = Helper_ConvertIntToArr(x_value);
  int *y_array = Helper_ConvertIntToArr(y_value);

  for (int i = 0; i < ANALOG_SIZE_CONST; i++) {
    // 1000 - first four digit number
    if (x_value < 1000) {
      if (i == 0) {
        jxy_msg[1] = '0';
      }
    } else {
      jxy_msg[i + 1] = (char)(x_array[i] + '0');
      Serial.println("x_array:");
      Serial.println((char)(x_array[i] + '0'));
    }
  }

  int second_value_start{ANALOG_SIZE_CONST + 1};
  int j{second_value_start};

  for (int i = 0; i < ANALOG_SIZE_CONST; i++, j++) {
    // 1000 - first four digit number
    if (y_value < 1000) {
      if (j == ANALOG_SIZE_CONST + 1) {
        jxy_msg[second_value_start] = '0';
      }
    } else {
      jxy_msg[j] = (char)(y_array[i] + '0');
    }
  }

Serial.println("[CreateMessageJoystickXY] Message:");
  for(int i = 0; i < JOYSTICK_XY_MSG_LENGTH; i++){
  Serial.print(jxy_msg[i]);
  Serial.println();
  }

  delete[] x_array;
  delete[] y_array;
}

/// @brief Create joystick-button-info message for Radio transmit
/// @param button_status bool
/// @param msg char*
void CreateMessageJoystickButton(bool button_status, char *msg) {
  msg[0] = JOYSTICK_BUTTON_MSG_HEADER;

  if (button_status) {
    msg[1] = '1';
  } else {
    msg[1] = '0';
  }
}

/// @brief Create button-info message for Radio transmit
/// @param msg char*
void CreateMessageButton(char *msg) {
  msg[0] = BUTTON_MSG_HEADER;

  if (lastButtonState) {
    msg[1] = '1';
  } else {
    msg[1] = '0';
  }
}

/// @brief Transmit created messages to the receiver
void TransmitData() {
#ifdef DEBUG_ENABLED
  Serial.println("CreateMessageJoystickXY");
#endif
  driver.send((uint8_t *)jxy_msg, strlen(jxy_msg));
#ifdef DEBUG_ENABLED
  Serial.println("[TransmitData] Message:");
  Serial.println(jxy_msg[0]); // H
  Serial.println(jxy_msg[1]); // X
  Serial.println(jxy_msg[2]); // X
  Serial.println(jxy_msg[3]); // X
  Serial.println(jxy_msg[4]); // X
  Serial.println(jxy_msg[5]); // Y
  Serial.println(jxy_msg[6]); // Y
  Serial.println(jxy_msg[7]); // Y
  Serial.println(jxy_msg[8]); // Y
#endif

  delay(1000);
}

/// @brief Loop
void loop() {
  int x = JoyStickGetX();
  int y = JoyStickGetY();
#ifdef DEBUG_ENABLED
  Serial.println("Joystick X");
  Serial.println(x);
  Serial.println("Joystick Y");
  Serial.println(y);
#endif
  CreateMessageJoystickXY(1111, 2222);
  TransmitData();
}
