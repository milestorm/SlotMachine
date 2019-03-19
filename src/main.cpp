#include <Arduino.h>
#include <MaxMatrix.h>
#include <avr/pgmspace.h>

PROGMEM const unsigned char CH[] = {
8, 8, B00111110, B01000001, B01000001, B00111110, B00000000, B00111110, B01000001, B01000001, // 0
8, 8, B01000010, B01111111, B01000000, B00000000, B00000000, B01000010, B01111111, B01000000, // 1
8, 8, B01100010, B01010001, B01001001, B01000110, B00000000, B01100010, B01010001, B01001001, // 2
8, 8, B00100010, B01000001, B01001001, B00110110, B00000000, B00100010, B01000001, B01001001, // 3
8, 8, B00011000, B00010100, B00010010, B01111111, B00000000, B00011000, B00010100, B00010010, // 4
8, 8, B00100111, B01000101, B01000101, B00111001, B00000000, B00100111, B01000101, B01000101, // 5
8, 8, B00111110, B01001001, B01001001, B00110000, B00000000, B00111110, B01001001, B01001001, // 6
8, 8, B01100001, B00010001, B00001001, B00000111, B00000000, B01100001, B00010001, B00001001, // 7
8, 8, B00110110, B01001001, B01001001, B00110110, B00000000, B00110110, B01001001, B01001001, // 8
8, 8, B00000110, B01001001, B01001001, B00111110, B00000000, B00000110, B01001001, B01001001, // 9
2, 8, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, // : as space
};

// Dot Matrix pin definitions
#define DOTMATRIX_DIN 4
#define DOTMATRIX_CS  3
#define DOTMATRIX_CLK 2
#define DOTMATRIX_DISPLAY_COUNT 1
MaxMatrix dot_matrix(DOTMATRIX_DIN, DOTMATRIX_CS, DOTMATRIX_CLK, DOTMATRIX_DISPLAY_COUNT);

byte xicht_happy[] = {B11111100, B00100000, B01001100, B01000000, B01000000, B01001100, B00100000, B01010101};


// Scrollingtext helpers
byte buffer[10];
char start_message[] = "0:1:2:3:4:5:6:7:8:9:";  // Scrolling Text

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Put extracted character on Display
void printCharWithShift(char c, int shift_speed){
  if (c < 48) return;
  c -= 48;
  memcpy_P(buffer, CH + 10 * c, 10);
  dot_matrix.writeSprite(DOTMATRIX_DISPLAY_COUNT * 8, 0, buffer);
  dot_matrix.setColumn(DOTMATRIX_DISPLAY_COUNT * 8 + buffer[0], 0);

  for (int i = 0; i < buffer[0] + 1; i++)
  {
    delay(shift_speed);
    dot_matrix.shiftLeft(false, false);
  }
}

// Extract characters from Scrolling text
void printRollWithShift(char* s, int shift_speed){
    while (*s != 0){
        printCharWithShift(*s, shift_speed);
        s++;
    }
}



void setup() {
    Serial.begin(115200);

    // Initialize MaxMatrix
    dot_matrix.init(); // module initialize
    dot_matrix.setIntensity(1); // dot matix intensity 0-15
    dot_matrix.clear();

    Serial.println("Tohle je test");
}

void loop() {


    printRollWithShift(start_message, 100);  // Send scrolling Text

    while(1){
        /* endless stop */
    }

}