#include "cowpi.h"

void setup_simple_io();
void setup_matrix_keypad();
void setup_display_module();
uint8_t get_key_pressed();
void display_data(uint8_t address, uint8_t value);

struct gpio_registers *gpio;
struct spi_registers *spi;
unsigned long last_left_button_press = 0;
unsigned long last_right_button_press = 0;
unsigned long last_left_switch_slide = 0;
unsigned long last_right_switch_slide = 0;
unsigned long last_keypad_press = 0;
int64_t displayValue = 0;
int mode = 10;
uint8_t left_switch_last_position = 0;
uint8_t right_switch_last_position = 0;
uint8_t left_switch_current_position = (gpio[A0_A5].input & 0b00010000) >> 4;
uint8_t right_switch_current_position = (gpio[A0_A5].input & 0b00100000) >> 5;
uint8_t left_button_current_position = gpio[D8_D13].input & 0b00000001;
uint8_t right_button_current_position = gpio[D8_D13].input & 0b00000010;


// Layout of Matrix Keypad
//        1 2 3 A
//        4 5 6 B
//        7 8 9 C
//        * 0 # D
// This array holds the values we want each keypad button to correspond to
const uint8_t keys[4][4] = {
  {0x1,0x2,0x3,0xA},
  {0x4,0x5,0x6,0xB},
  {0x7,0x8,0x9,0xC},
  {0xF,0x0,0xE,0xD}
};

// Seven Segment Display mapping between segments and bits
// Bit7 Bit6 Bit5 Bit4 Bit3 Bit2 Bit1 Bit0
//  DP   A    B    C    D    E    F    G
// This array holds the bit patterns to display each hexadecimal numeral
const uint8_t seven_segments[16] = {
  0b01111110,//0
  0b00110000,//1
  0b01101101,//2
  0b01111001,//3
  0b00110011,//4
  0b01011011,//5
  0b01011111,//6
  0b01110000,//7
  0b01111111,//8
  0b01111011,//9
  0b01110111,//A
  0b00011111,//B
  0b00001101,//C
  0b00111101,//D
  0b01001111,//E
  0b01000111,//F
};

// #include "PollingLabTest.h"

double getDisplayValue(int valueAdded, int mode) {
  displayValue *= mode;
  if(displayValue < 0) {
    displayValue -= valueAdded;
  }
  else {
    displayValue += valueAdded;
  }
  return displayValue;
}
double negateValue() {
  displayValue *= -1;
}
void outputDisplay(int mode) {
  int64_t valueCopy = displayValue;
  if(displayValue < 0 && mode == 10) {
    valueCopy *= -1;
  }
  else if(displayValue < 0 && mode == 16) {
    valueCopy &= 0x00000000ffffffff;
  }
  
  for(int digit = 8; digit != 0; digit--) {
    int64_t digitValue = 1;
    int segmentValue = 0;
    for(int i = 0; i < digit - 1; i++) {
      digitValue *= mode;
    }
    
    while(valueCopy - (segmentValue * digitValue) >= 0) {
      segmentValue++;
    }
    segmentValue--;
    valueCopy -= (segmentValue * digitValue);
    if(valueCopy == displayValue || valueCopy == (displayValue * -1)) {
      if(displayValue < 0 && mode == 10) {
        if(digit != 8) {
        display_data(digit + 1, 0x00);
       }
       display_data(digit, 0b00000001);
      }
    }
    else {
      display_data(digit, seven_segments[segmentValue]);
    }
  }
}

void clearDisplay() {
  display_data(1, 0x00);
  display_data(2, 0x00);
  display_data(3, 0x00);
  display_data(4, 0x00);
  display_data(5, 0x00);
  display_data(6, 0x00);
  display_data(7, 0x00);
  display_data(8, 0x00);
}

void setup() {
  Serial.begin(9600);
  gpio = (gpio_registers*)(IObase + 3);
  spi = (spi_registers*)(IObase + 0x2C);
  displayValue = 0;
  setup_simple_io();
  setup_keypad();
  setup_display_module();
}

void loop() {
  left_switch_current_position = (gpio[A0_A5].input & 0b00010000) >> 4;
  right_switch_current_position = (gpio[A0_A5].input & 0b00100000) >> 5;
  left_button_current_position = gpio[D8_D13].input & 0b00000001;
  right_button_current_position = gpio[D8_D13].input & 0b00000010;
  
   if ((left_switch_current_position != left_switch_last_position)) {
      clearDisplay();
      displayValue = 0;
      //left_switch_last_position = left_switch_current_position;
   }
   if (right_button_current_position == 0 && (millis() - last_right_button_press) > 500) {
      clearDisplay();
      //displayValue = 0;
      
      if(mode == 10) {
        mode = 16;
      }
      else {
        mode = 10;
      }
      if(mode == 10 && (displayValue > 99999999 || displayValue < -9999999)) {
        display_data(1, 0b00000101);
        display_data(2, 0b00011101);
        display_data(3, 0b00000101);
        display_data(4, 0b00000101);
        display_data(5, 0b01001111);
        display_data(6, 0b00000000);
        display_data(7, 0b00000000);
        display_data(8, 0b00000000);
      }
      else {
        outputDisplay(mode);
      }
      last_right_button_press = millis();
   }
   if(left_button_current_position == 0 && (millis() - last_left_button_press) > 500) {
    clearDisplay();
    negateValue();
    outputDisplay(mode);
   }
      
     test_simple_io();

     if(left_switch_current_position == 0) {
        if(millis() - last_keypad_press > 500) {
      gpio[D8_D13].output &= 0b11101111;
     }
     if (((gpio[A0_A5].input & 0b00001111) != 0b00001111) && (millis() - last_keypad_press > 500)) {
       gpio[D8_D13].output |= 0b00010000;
       uint8_t keypress = get_key_pressed();
       if (keypress < 0x10) {
         Serial.print("Key pressed: ");
         Serial.println(keypress, HEX);
         display_data(1, seven_segments[keypress]);
      } else {
         Serial.println("Error reading keypad.");
       }
     }
   }
   else {
    //if right switch == 1, mode = 16
    //else mode = 10
    if (((gpio[A0_A5].input & 0b00001111) != 0b00001111) && (millis() - last_keypad_press > 500)) {
      gpio[D8_D13].output |= 0b00010000;
       uint8_t keypress = get_key_pressed();
       if(mode == 10) {
        if(keypress < 10) {
          Serial.print("Key pressed: ");
          Serial.println(keypress, HEX);
          getDisplayValue(keypress, mode);
          if(displayValue > 99999999 || displayValue < -9999999) {
            display_data(1, 0b01011110);
            display_data(2, 0b00000110);
            display_data(3, 0b00011111);
            display_data(4, 0b00000000);
            display_data(5, 0b00011101);
            display_data(6, 0b00011101);
            display_data(7, 0b00001111);
            display_data(8, 0b00000000);
          }
          else {
            outputDisplay(mode);
          }
        }
          else {
            Serial.print("Key pressed: ");
            Serial.println(keypress, HEX);
          }
       }
       else {
        if(displayValue > 0xefffffff || displayValue < -2147483648) {
          display_data(1, 0b01011110);
          display_data(2, 0b00000110);
          display_data(3, 0b00011111);
          display_data(4, 0b00000000);
          display_data(5, 0b00011101);
          display_data(6, 0b00011101);
          display_data(7, 0b00001111);
          display_data(8, 0b00000000);
        }
        else {
          Serial.print("Key pressed: ");
          Serial.println(keypress, HEX);
          getDisplayValue(keypress, mode);
          outputDisplay(mode);
        }
       }
    }
   }
  }


void setup_simple_io() {
  gpio[A0_A5].input &= 0b11111111;
  gpio[A0_A5].output &= 0b11001111;
  gpio[A0_A5].direction &= 0b11001111;

  gpio[D8_D13].input &= 0b11111111;
  gpio[D8_D13].output |= 0b00000011;
  gpio[D8_D13].direction &= 0b11111100;

  gpio[D8_D13].direction |= 0b00010000;
}

void setup_keypad() {
  gpio[D0_D7].direction |= 0b11110000;
  gpio[D0_D7].output &= 0b00001111;
  
  gpio[A0_A5].direction &= 0b11110000;
  gpio[A0_A5].output |= 0b00001111;
}
void setup_display_module() {
  gpio[D8_D13].direction |= 0b00101100;
  spi->control |= 0b01010011;
  spi->control &= 0b01010011;
  for (char i = 1; i <= 8; i++) {
    display_data(i, 0);     // clear all digit registers
  }
  display_data(0xA, 8);     // intensity at 17/32
  display_data(0xB, 7);     // scan all eight digits
  display_data(0xC, 1);     // take display out of shutdown mode
  display_data(0xF, 0);     // take display out of test mode, just in case
}

uint8_t get_key_pressed() {
  uint8_t key_pressed = 0xFF;
  unsigned long now = millis();
  if (now - last_keypad_press > 500) {
    last_keypad_press = now;
    for(int i = 0; i < 4; i++) {
      gpio[D0_D7].output |= 0b11110000;
      gpio[D0_D7].output ^= 0b00010000 << i;

      if(!(gpio[A0_A5].input & 0b00000001)) {
        key_pressed = keys[i][0];
      }
      else if(!(gpio[A0_A5].input & 0b00000010)) {
        key_pressed = keys[i][1];
      }
      else if(!(gpio[A0_A5].input & 0b00000100)) {
        key_pressed = keys[i][2];
      }
      else if(!(gpio[A0_A5].input & 0b00001000)) {
        key_pressed = keys[i][3];
      }
    }
    gpio[D0_D7].output &= 0b00001111;
  }
  return key_pressed;
}

void display_data(uint8_t address, uint8_t value) {
  // address is MAX7219's register address (1-8 for digits; otherwise see MAX7219 datasheet Table 2)
  // value is the bit pattern to place in the register
  gpio[D8_D13].output &= 0b11111011;
  spi->data = address;
  while(!(spi->status & 0b10000000)){}
  spi->data = value;
  while(!(spi->status & 0b10000000)){}
  gpio[D8_D13].output |= 0b00000100;
}

//uint8_t left_switch_last_position = 0;
//uint8_t right_switch_last_position = 0;

void test_simple_io() {
  uint8_t printed_this_time = 0;
  left_switch_current_position = (gpio[A0_A5].input & 0b00010000) >> 4;
  right_switch_current_position = (gpio[A0_A5].input & 0b00100000) >> 5;
  left_button_current_position = gpio[D8_D13].input & 0b00000001;
  right_button_current_position = gpio[D8_D13].input & 0b00000010;
  unsigned long now = millis();
  if ((left_switch_current_position != left_switch_last_position) && (now - last_left_switch_slide > 500)) {
    Serial.print(now);
    Serial.print("\tLeft switch changed: ");
    Serial.print(left_switch_current_position);
    left_switch_last_position = left_switch_current_position;
    printed_this_time = 1;
    last_left_switch_slide = now;
  }
  if ((right_switch_current_position != right_switch_last_position) && (now - last_right_switch_slide > 500)) {
    if (!printed_this_time) {
      Serial.print(now);
    }
    Serial.print("\tRight switch changed: ");
    Serial.print(right_switch_current_position);
    right_switch_last_position = right_switch_current_position;
    printed_this_time = 1;
    last_right_switch_slide = now;
  }
  if (!left_button_current_position && (now - last_left_button_press > 500)) {
    if (!printed_this_time) {
      Serial.print(now);
    }
    Serial.print("\tLeft button pressed");
    printed_this_time = 1;
    last_left_button_press = now;
  }
  if (!right_button_current_position && (now - last_right_button_press > 500)) {
    if (!printed_this_time) {
      Serial.print(now);
    }
    Serial.print("\tRight button pressed");
    printed_this_time = 1;
    last_right_button_press = now;
  }
  if(printed_this_time) {
    Serial.println();
  }
  //digitalWrite(12, left_switch_current_position && right_switch_current_position);
  /*if(left_switch_current_position & right_switch_current_position) {
    gpio[D8_D13].output |= 0b00010000;
  }
  else {
    gpio[D8_D13].output &= 0b11101111;
  }*/
}
