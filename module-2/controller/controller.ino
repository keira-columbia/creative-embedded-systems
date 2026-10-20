// Your Turn controller: send the controls to Processing over USB.
// Based on the course Lab 2 examples. Power the sensors from 3.3V.
const int joystickXPin = 39;
const int joystickYPin = 32;
const int joystickSwitchPin = 33;
const int potentiometerPin = 13;
const int buttonPin = 2;
const int ledPin = 15;

void setup() {
  Serial.begin(9600);
  pinMode(joystickSwitchPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(potentiometerPin, INPUT);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  int x = analogRead(joystickXPin);
  int y = analogRead(joystickYPin);
  int knob = analogRead(potentiometerPin);
  int joystickSwitch = digitalRead(joystickSwitchPin);
  int button = digitalRead(buttonPin);

  // The separate button lights the LED while held.
  digitalWrite(ledPin, button == LOW ? HIGH : LOW);

  // Columns: joystick X, joystick Y, joystick click, knob, button.
  Serial.printf("%d,%d,%d,%d,%d\n", x, y, joystickSwitch, knob, button);
  delay(50);
}
