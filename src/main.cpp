#include <Arduino.h>
#include <MaxMatrix.h>
#include <avdweb_VirtualDelay.h>
#include <avr/pgmspace.h>

// Symbol definitions
PROGMEM const unsigned char CH[] = {
8, 8, B00000110, B01101111, B11110110, B01100100, B00100100, B00101000, B00110000, B00100000, // 0: cherry
8, 8, B00000000, B00111100, B01111110, B11111101, B01100010, B00111100, B00000000, B00000000, // 1: lemon
8, 8, B00111100, B01111110, B11111111, B11111111, B11111101, B11111101, B01100010, B00111100, // 2: orange
8, 8, B00000000, B00011110, B01111111, B11111101, B11111010, B11100100, B01111000, B00000000, // 3: plum
8, 8, B00011000, B11111111, B01111010, B01111010, B01111010, B01111010, B00110100, B00011000, // 4: bell
8, 8, B00011000, B00111100, B00111100, B01111110, B01111110, B00011000, B00010000, B00100000, // 5: grape
8, 8, B00011100, B00111110, B01111101, B11111010, B11110100, B11101000, B01010000, B00100000, // 6: melon
8, 8, B00011000, B00011000, B00011000, B00011000, B00110000, B01100000, B01111110, B01111110, // 7: seven
8, 8, B01000010, B00100100, B00011000, B00111100, B11111111, B00011000, B00010000, B00010000, // 8: star
8, 8, B00111100, B01000010, B10011001, B10100101, B10000001, B10100101, B01000010, B00111100, // 9: joker
};

// Symbols pay value (0 to 9), for bets x1
int symbolValue[10] = {4, 4, 8, 8, 12, 16, 20, 30, 40, 75};

// Dot Matrix pin definitions
#define DOTMATRIX_DIN 4
#define DOTMATRIX_CS  3
#define DOTMATRIX_CLK 2
#define DOTMATRIX_DISPLAY_COUNT 1

byte xicht_happy[] = {8, 8, B10000001, B01000010, B00100100, B00011000, B00011000, B00100100, B01000010, B10000001};

// Cylinder definitions
int cylinder[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 7, 7, 5, 3, 4, 5, 3, 5, 8};




//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

class SlotCylinder {
	byte buffer[10];

	int *p_cylinderArr; // cylinder array (numbers 0 to 9)
	int arrSize; // size of cylinderArr
	int shiftSpeed; // speed of scrolling. Best is 25
	int shiftCount; // how many times will the cylinder turn. More like symbol shift.
	int startingIndex; // the index in cylinderArr where the cylinder will start

	int realIndex = 0; // real index of position in cylinder
	int realShiftSpeed; // shift speed already with slow start
	int shiftStart[18] = {150,149,145,140,132,123,113,101,88,75,62,49,38,27,18,10,5,1}; // slow start array. Like fade in
	int shiftStartLength = 18; // length of shiftStart array. the slowing array.
	int shiftStartPosition; // position for slow start

	bool isRolling = false;

	VirtualDelay vdelay; // initialize virtual delay
	MaxMatrix *p_dotMatrix;

	//constructor
	public:
	/**
	 * Constructor of SlotCylinder
	 * @param {int} *_p_cylinderArr pointer to array
	 * @param {int} _arrSize size of the array
	 * @param {int} _shiftSpeed speed of shift. Best is 25
	 */
	SlotCylinder(int *_p_cylinderArr, int _arrSize, int _shiftSpeed) {
		p_cylinderArr = _p_cylinderArr;
		arrSize = _arrSize;
		shiftSpeed = _shiftSpeed;
		realShiftSpeed = _shiftSpeed;
	}

	// Initialize MaxMatrix
	void initMatrix(byte din, byte cs, byte clk) {
		p_dotMatrix = new MaxMatrix(din, cs, clk, 1);
		p_dotMatrix->init(); // module initialize
    p_dotMatrix->setIntensity(1); // dot matix intensity 0-15
    p_dotMatrix->clear();
    p_dotMatrix->writeSprite(0, 0, xicht_happy);
	}

	/**
	 * Rolls the cylinder
	 * @param {int} _shiftCount how many symbols will be shifted
	 */
	void roll(int _shiftCount) {
		shiftCount = _shiftCount;
		startingIndex = realIndex;
		shiftStartPosition = 0;

		isRolling = true;

		// set right speed based on actual shiftspeed
		for (int sb = 0; sb < shiftStartLength; sb++) {
			shiftStart[sb] = shiftStart[sb] + shiftSpeed;
		}

		for (int mainCount = 0; mainCount < shiftCount; mainCount++) { // shift x times

			if (realIndex >= arrSize) { // endless shift thru the cylinder array
				realIndex = realIndex - arrSize;
			}

			// display symbol to hidden place
			memcpy_P(buffer, CH + 10 * p_cylinderArr[realIndex], 10);
			p_dotMatrix->writeSprite(9, 0, buffer); // writes to 9th position because of 1px space
			p_dotMatrix->setColumn(9 + buffer[0], 0);

			for (int i = 0; i < buffer[0] + 1; i++) { // shift from hidden place
				// makes the starting speed slower
				if (shiftStartPosition < shiftStartLength) {
					realShiftSpeed = shiftStart[shiftStartPosition];
				}	else {
					realShiftSpeed = shiftSpeed;
				}

				delay(realShiftSpeed); // REWRITE TO MILLIS!!!
				p_dotMatrix->shiftLeft(false, false);

				shiftStartPosition++;
			}

			realIndex++;

		}

		isRolling = false;

	}

	int getPosition() {
		return p_cylinderArr[realIndex - 1];
	}


};




SlotCylinder cylinder1(cylinder, 20, 25);
SlotCylinder cylinder2(cylinder, 20, 25);

void setup() {
    Serial.begin(115200);

		cylinder1.initMatrix(4, 3, 2);
		cylinder2.initMatrix(5, 6, 7);
}

void loop() {

		delay(6000);

		cylinder1.roll(10);
		cylinder2.roll(17);

		Serial.print("CYL1: ");
		Serial.println(cylinder1.getPosition());

		delay(3000);

		cylinder1.roll(6);
		cylinder2.roll(10);

		Serial.print("CYL1: ");
		Serial.println(cylinder1.getPosition());

		delay(3000);

		cylinder1.roll(33);
		cylinder2.roll(38);

		Serial.print("CYL1: ");
		Serial.println(cylinder1.getPosition());



    while(1){
        /* endless stop */
    }

}