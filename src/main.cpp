#include <Arduino.h>
#include <MaxMatrix.h>
#include <avdweb_VirtualDelay.h>
#include <avr/pgmspace.h>
#include <OneButton.h>

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

// Cylinder definitions
int cylinderSymbols1[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 7, 7, 5, 3, 4, 5, 3, 5, 8};


byte startingPicture[] = {8, 8, B10000001, B01000010, B00100100, B00011000, B00011000, B00100100, B01000010, B10000001};


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
	int shiftStartPosition = 0; // position for slow start

	int *rollingArray = NULL;
	int rollingArrayIndex = 0;
	int rollingArrayLength = 0;

	bool isRolling = false;

	VirtualDelay rollDelay; // initialize virtual delay
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

		// set right speed based on actual shiftspeed
		for (int sb = 0; sb < shiftStartLength; sb++) {
			shiftStart[sb] = shiftStart[sb] + shiftSpeed;
		}
	}

	// Initialize MaxMatrix
	void initMatrix(byte din, byte cs, byte clk) {
		p_dotMatrix = new MaxMatrix(din, cs, clk, 1);
		p_dotMatrix->init(); // module initialize
    p_dotMatrix->setIntensity(1); // dot matix intensity 0-15
    p_dotMatrix->clear();
    p_dotMatrix->writeSprite(0, 0, startingPicture);
	}

	bool isCylinderRolling() {
		return isRolling;
	}

	/**
	 * Generates shifting array for tne roll
	 */
	void generateShiftArray(int _count) {
		int count = _count * 9;
		rollingArrayLength = count;

		if (*rollingArray != NULL) {
			Serial.println("deleting rollingArray");
			delete rollingArray;
		}

		rollingArray = new int[count];

		for (int i = 0; i < count; i++) {
			// makes the starting speed slower
			if (shiftStartPosition < shiftStartLength) {
				rollingArray[i] = shiftStart[shiftStartPosition];
			}	else {
				rollingArray[i] = shiftSpeed;
			}
			shiftStartPosition++;
		}

		//return *rollingArray;
	}

	void update() {
		int updDelay;
		if (isRolling == true) {
			updDelay = *(rollingArray + rollingArrayIndex);
			if (updDelay > *(rollingArray + 0) || (updDelay < 0)) {
				updDelay = shiftSpeed;
			}

			rollDelay.start(updDelay);
			if (rollDelay.elapsed()) {
				Serial.print("TICK ");
				Serial.print(*(rollingArray + rollingArrayIndex));
				Serial.print(" -- ");
				Serial.print(rollingArrayIndex);
				Serial.print(" : ");
				Serial.println(*rollingArray);

				// rolling has finished
				if ((rollingArrayLength-1) < rollingArrayIndex) {
					isRolling = false;
					rollingArrayIndex = 0;
					Serial.println("!!!! Finished rolling...");
				} else {
					if (rollingArrayIndex % 9 == 0) { // time to print symbol
						Serial.println("### SYMBOL ###");
						if (realIndex >= arrSize) { // endless shift thru the cylinder array
							realIndex = realIndex - arrSize;
						}
						// display symbol to hidden place
						memcpy_P(buffer, CH + 10 * p_cylinderArr[realIndex], 10);
						p_dotMatrix->writeSprite(9, 0, buffer); // writes to 9th position because of 1px space
						p_dotMatrix->setColumn(9 + buffer[0], 0);

						realIndex++;
					}

					p_dotMatrix->shiftLeft(false, false);
					rollingArrayIndex++;
				}


			}

		}

	}

	void fakeRoll() {
		isRolling = true;
	}

	/**
	 * Rolls the cylinder
	 * @param {int} _shiftCount how many symbols will be shifted
	 */
	void roll(int _shiftCount) {
		isRolling = true;
		shiftCount = _shiftCount;
		startingIndex = realIndex;
		shiftStartPosition = 0;

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




SlotCylinder cylinder1(cylinderSymbols1, 20, 25);
SlotCylinder cylinder2(cylinderSymbols1, 20, 25);

OneButton startButton(A1, true);

bool slotRunning = false;
int credit = 100;
int bet = 10;




void startButtonFn() {
	// random generate shift array length within some boundaries.
	// 1st cylinder shortest, than 2nd and 3rd longest
	cylinder1.generateShiftArray(5);
	cylinder2.generateShiftArray(6);
	cylinder1.fakeRoll();
	cylinder2.fakeRoll();

	slotRunning = true;
}

void slotWatch() {
	if (slotRunning == true) {
		if (cylinder1.isCylinderRolling() == false && cylinder2.isCylinderRolling() == false) {
			Serial.println("###############");
			Serial.print("CYL 1: ");
			Serial.println(cylinder1.getPosition());
			Serial.print("CYL 2: ");
			Serial.println(cylinder2.getPosition());
			Serial.println("---------------");
			/* TODO
				- jokers.
				- first two symbols pays as 0.5 bet MAYBE
				- if credit = 0 game over, insert coin
			*/

			if (cylinder1.getPosition() == cylinder2.getPosition()) {
				// WIN
				credit += (symbolValue[cylinder1.getPosition()] * bet);
				Serial.println("*** WINNER ***");
				Serial.print("credit: ");
				Serial.println(credit);
			} else {
				// LOSE
				credit -= bet;
				Serial.println("-_- LOSER -_-");
				Serial.print("credit: ");
				Serial.println(credit);
			}




			slotRunning = false;
		}
	}
}

void setup() {
    Serial.begin(115200);

		// init display 8x8 matrix with DIN, CS, CLK
		cylinder1.initMatrix(4, 3, 2);
		cylinder2.initMatrix(5, 6, 7);

		startButton.attachClick(startButtonFn);

}

void loop() {

		startButton.tick();

		cylinder1.update();
		cylinder2.update();

		slotWatch();

}