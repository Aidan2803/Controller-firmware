// Version 1.1.0 Change transmission type to transmit one main message

#include <RH_ASK.h>
#include <SPI.h>

#define DEBUG_ENABLED
#define DEBUG_JOYSTICK_ENABLED
#define DEBUG_BUTTON_ENABLED

RH_ASK driver;

/* Transmit message creation variables */

///@brief  Maximum possible amout of digits in the analog value
const int ANALOG_SIZE_CONST{4};

/// @brief Lenght of a joystick XY message (in bytes)
const int JOYSTICK_XY_MSG_LENGTH{9}; // HXXXXYYYY
/// @brief Lenght of a joystick button message (in bytes)
const int JOYSTICK_BUTTON_MSG_LENGTH{2}; // HV
/// @brief Lenght of a button message (in bytes)
const int BUTTON_MSG_LENGTH{2}; // HV

const short JOYSTICK_XY_MSG_HEADER{0};
const short JOYSTICK_BUTTON_MSG_HEADER{1};
const short BUTTON_MSG_HEADER{2};

char *jxy_msg{};
char *jbutt_msg{};
char *butt_msg{};
char *main_msg{};

/* Pins */

/* RH transmitter */

const int RH_TransPin = 12;

/* Joystick pins */
const int joystickPinX = 0;    // A0
const int joystickPinY = 1;    // A1
const int joystickPinButt = 4; // D2

/* Button pins */
const int buttonPin = 2; // D7

/* Button variables */
int buttonState = 0;
int lastButtonState = 0;
int lastDebounceTime = 0;
int debounceDelay = 0;

/// @brief Helper function to convert int into int array: 1234 -> |1|2|3|4|
/// @param value
/// @return int* (array pointer)
int *Helper_ConvertIntToArr(int value, int *digits_amount) {
  int value_copy{value};
  int digits_in_the_number{1};

  while (value / 10 != 0) {
    value /= 10;
    digits_in_the_number++;
  }

  int *result_digits = new int[ANALOG_SIZE_CONST];
  *digits_amount = digits_in_the_number;

  for (int i = 0; i < ANALOG_SIZE_CONST; i++) {
    result_digits[i] = 0;
  }

  int digits_in_the_number_cp = digits_in_the_number;

  for (int i = 0; digits_in_the_number > 0; i++) {
#ifdef DEBUG_ENABLED
    Serial.print("[Helper_ConvertIntToArr] digits_in_the_number:");
    Serial.println(digits_in_the_number);
#endif

    int pow10{1};
    for (int i = 0; i < digits_in_the_number - 1; i++) {
      pow10 *= 10;
    }

    result_digits[i] = value_copy / pow10;

    int mult_res = result_digits[i] * pow10;
    value_copy = value_copy - mult_res;

    digits_in_the_number--;
  }

  // Delta between expected amount of digits (ANALOG_SIZE_CONST = 4) and actual
  // amout of digits
  int delta_digits = ANALOG_SIZE_CONST - digits_in_the_number_cp;

  for (int j = 0; j < delta_digits; j++) {
    for (int i = ANALOG_SIZE_CONST - 1; i > 0; i--) {
      int temp = result_digits[i - 1];
      result_digits[i - 1] = result_digits[i];
      result_digits[i] = temp;
    }
  }

#ifdef DEBUG_ENABLED
  Serial.println("[Helper_ConvertIntToArr] Reslt digits:");
  for (int i = 0; i < ANALOG_SIZE_CONST; i++) {
    Serial.print(result_digits[i]);
  }
  Serial.println();
#endif
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
  pinMode(joystickPinButt, INPUT_PULLUP);
  pinMode(buttonPin, INPUT);

  jxy_msg = new char[JOYSTICK_XY_MSG_LENGTH];
  jbutt_msg = new char[JOYSTICK_BUTTON_MSG_LENGTH];
  butt_msg = new char[BUTTON_MSG_LENGTH];
  int main_msg_length{JOYSTICK_XY_MSG_LENGTH + JOYSTICK_BUTTON_MSG_LENGTH +
                      BUTTON_MSG_LENGTH};
  main_msg = new char[main_msg_length];
}

/// @brief Get X value from joystick
/// @return int Joystick X (analog 0-1023)
int JoyStickGetX() { return analogRead(joystickPinX); }

/// @brief Get Y value from joystick
/// @return int Joystick Y (analog 0-1023)
int JoyStickGetY() { return analogRead(joystickPinY); }

/// @brief Get button value from joystick
/// @return bool Joystick Button (digital 0-1)
bool JoyStcikIsButtonPressed() { return !digitalRead(joystickPinButt); }

/// @brief Check if button is pressed
void ButtonIsPressed() {
  int currentButtonState = digitalRead(buttonPin);
#ifdef DEBUG_BUTTON_ENABLED
  Serial.println("[ButtonIsPressed] currentButtonState:");
  Serial.println(currentButtonState);
#endif

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
#ifdef DEBUG_JOYSTICK_ENABLED
  Serial.print("[CreateMessageJoystickXY] Received x: ");
  Serial.println(x_value);
  Serial.print("[CreateMessageJoystickXY] Received y: ");
  Serial.println(y_value);
#endif
  jxy_msg[0] = JOYSTICK_XY_MSG_HEADER;

  int *x_value_digits_amount = new int;
  int *y_value_digits_amount = new int;

  int *x_array = Helper_ConvertIntToArr(x_value, x_value_digits_amount);
  int *y_array = Helper_ConvertIntToArr(y_value, y_value_digits_amount);

  for (int i = 0, j = 1; i < ANALOG_SIZE_CONST; i++, j++) {
    jxy_msg[j] = x_array[i];
  }

  int second_value_start{ANALOG_SIZE_CONST + 1};

  for (int i = 0; i < ANALOG_SIZE_CONST; i++) {
    jxy_msg[second_value_start + i] = y_array[i];
  }

  delete[] x_array;
  delete[] y_array;
}

/// @brief Create joystick-button-info message for Radio transmit
/// @param button_status bool
void CreateMessageJoystickButton(bool button_status) {
  jbutt_msg[0] = JOYSTICK_BUTTON_MSG_HEADER;

  if (button_status) {
    jbutt_msg[1] = 1;
  } else {
    jbutt_msg[1] = 0;
  }
}

/// @brief Create button-info message for Radio transmit
void CreateMessageButton() {
  butt_msg[0] = BUTTON_MSG_HEADER;

  if (lastButtonState) {
    butt_msg[1] = 1;
  } else {
    butt_msg[1] = 0;
  }
}

void PackData() {
  for (int i = 0; i < JOYSTICK_XY_MSG_LENGTH; i++) {
    main_msg[i] = jxy_msg[i];
  }
  for (int i = JOYSTICK_XY_MSG_LENGTH, j = 0; j < JOYSTICK_BUTTON_MSG_LENGTH;
       i++, j++) {
    main_msg[i] = jbutt_msg[j];
  }
  for (int i = JOYSTICK_XY_MSG_LENGTH + JOYSTICK_BUTTON_MSG_LENGTH, j = 0;
       j < BUTTON_MSG_LENGTH; j++) {
    main_msg[i] = butt_msg[j];
  }
};

/// @brief Transmit created messages to the receiver
void TransmitData() {
  driver.send((uint8_t *)main_msg, strlen(main_msg));
#ifdef DEBUG_JOYSTICK_ENABLED
  Serial.println("[TransmitData-JoystickXY] Message:");
  Serial.println((char)jxy_msg[0] + 0); // H
  Serial.println((char)jxy_msg[1] + 0); // X
  Serial.println((char)jxy_msg[2] + 0); // X
  Serial.println((char)jxy_msg[3] + 0); // X
  Serial.println((char)jxy_msg[4] + 0); // X
  Serial.println((char)jxy_msg[5] + 0); // Y
  Serial.println((char)jxy_msg[6] + 0); // Y
  Serial.println((char)jxy_msg[7] + 0); // Y
  Serial.println((char)jxy_msg[8] + 0); // Y

  Serial.println("[TransmitData-JoystickButton] Message:");
  Serial.println((char)jbutt_msg[0] + 0); // H
  Serial.println((char)jbutt_msg[1] + 0); // V
#endif

#ifdef DEBUG_BUTTON_ENABLED
  Serial.println("[TransmitData-Button] Message:");
  Serial.println((char)butt_msg[0] + 0); // H
  Serial.println((char)butt_msg[1] + 0); // V
#endif

  delay(1000);
}

/// @brief Main loop
void loop() {
  int x = JoyStickGetX();
  int y = JoyStickGetY();
  bool joy_stick_button_state = JoyStcikIsButtonPressed();
  ButtonIsPressed();

#ifdef DEBUG_JOYSTICK_ENABLED
  Serial.println("Joystick X");
  Serial.println(x);
  Serial.println("Joystick Y");
  Serial.println(y);
#endif

  CreateMessageJoystickXY(x, y);
  CreateMessageJoystickButton(joy_stick_button_state);
  CreateMessageButton();
  PackData();
  TransmitData();
}
