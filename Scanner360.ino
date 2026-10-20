#include <Arduino.h>
#include <EEPROM.h>
#include <multiCameraIrControl.h>
#include "logo.h"

//OLED
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET 4      // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Encoder
#define outputA 2
#define outputB 4
#define buttonPin 3
int counter = 0;
int aState;
int aLastState;

//Driver 4988
#include "BasicStepperDriver.h"
#define MOTOR_STEPS 6125
#define RPM 10
#define MICROSTEPS 2
#define DIR 9
#define STEP 8
BasicStepperDriver stepper(MOTOR_STEPS, DIR, STEP);

//Librairie APN Diode IR
Canon D5(11);

//Variales diverses
boolean do_update_screen = true;
int mode_menu = 0;
int nombre_photos = EEPROM.read(1);
int delai_photos = EEPROM.read(5) * 100;

void setup() {
  // Encoder
  pinMode(outputA, INPUT_PULLUP);
  pinMode(outputB, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  aLastState = digitalRead(outputA);

  //OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  //Stepper
  stepper.begin(RPM, MICROSTEPS);

  //Affichage du logo
  display.clearDisplay();
  display.drawBitmap(0, 0, fan_off, 128, 64, 1);
  display.display();
  delay(5000);
}

void loop() {
  //Bouton validation
  if (digitalRead(buttonPin) == LOW) {
    mode_menu++;
    delay(500);
    do_update_screen = true;
  }

  //Rotary et ajustement de valeur en fonction du mode
  if (mode_menu == 0) {
    nombre_photos = read_encoder(nombre_photos, 1);
    if (nombre_photos < 4) { nombre_photos = 4; }
    if (nombre_photos > 64) { nombre_photos = 64; }
  }
  if (mode_menu == 1) {
    delai_photos = read_encoder(delai_photos, 250);
    if (delai_photos < 0) { delai_photos = 0; }
    if (delai_photos > 5000) { delai_photos = 5000; }
  }

  //Actualisation de l'écran si nécessaire
  if (do_update_screen == true) {
    update_screen();
    do_update_screen = false;
  }

  //Cycle si mode 2 atteint
  if (mode_menu == 2) {
    cycle(nombre_photos, delai_photos);
    mode_menu = 0;
  }
}

int read_encoder(int initial_value, int increment) {
  aState = digitalRead(outputA);
  if (aState != aLastState) {
    if (digitalRead(outputB) != aState) {
      initial_value = initial_value + increment;
      do_update_screen = true;
    } else {
      initial_value = initial_value - increment;
      do_update_screen = true;
    }
  }
  aLastState = aState;
  return initial_value;
}

void update_screen() {
  int xpos = 10;
  int ypos = 0;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);  // Draw 2X-scale text
  display.setCursor(xpos, ypos);
  display.println(F("Nombre de Photos"));
  ypos += 9;
  display.setTextSize(2);  // Draw 2X-scale text
  if (mode_menu == 0) {
    display.setCursor(1, ypos);
    display.println(F(">"));
  }
  display.setCursor(xpos, ypos);
  display.println(nombre_photos);
  ypos += 15;
  display.setTextSize(1);  // Draw 2X-scale text
  display.setCursor(xpos, ypos);
  display.println(F("Delai prise photo"));
  ypos += 9;
  display.setTextSize(2);  // Draw 2X-scale text
  if (mode_menu == 1) {
    display.setCursor(1, ypos);
    display.println(F(">"));
  }
  display.setCursor(xpos, ypos);
  display.println(delai_photos);
  ypos += 15;
  display.setTextSize(2);  // Draw 2X-scale text
  if (mode_menu == 2) {
    display.setCursor(1, ypos);
    display.println(F(">"));
  }
  display.setCursor(xpos, ypos);
  display.println(F("Demarrer le scan"));
  display.display();  // Show initial text
}

void cycle(int nombre_photos, int delai_photos) {
  EEPROM.write(1, nombre_photos);
  EEPROM.write(5, delai_photos / 100);
  for (int i = 0; i < nombre_photos; i++) {
    stepper.rotate(360 / nombre_photos);
    digitalWrite(12, HIGH);
    D5.shutterNow();
    delay(delai_photos);
    digitalWrite(12, LOW);
    if (digitalRead(buttonPin) == LOW) {
      asm volatile(" jmp 0");
      break;
    }
  }
  mode_menu = 0;
  update_screen();
}
