#include <Arduino.h>
#include <MaxMatrix.h>
#include <avdweb_VirtualDelay.h>
#include <avr/pgmspace.h>
#include <OneButton.h>
#include <LiquidCrystal_I2C.h>
#include <EasyBuzzer.h>
#include <rtttl.h>

// PINs definitions
#define CYLINDER_1_DIN 4
#define CYLINDER_1_CS  5
#define CYLINDER_1_CLK 6

#define CYLINDER_2_DIN 7
#define CYLINDER_2_CS  8
#define CYLINDER_2_CLK 9

#define CYLINDER_3_DIN 14
#define CYLINDER_3_CS  16
#define CYLINDER_3_CLK 10

#define BUZZER 15
// set LCD to I2C pins. Arduino Leonardo Mini is  2: SDA, 3: SCL
#define BUTTON_START A2
#define BUTTON_BET A3
#define COIN_INSERTER A0

#define LED_START A1
//#define LED_BET A0

// ============================================================

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
int cylinderSymbols1[22] = {0, 1, 5, 3, 4, 9, 2, 7, 8, 9, 0, 7, 7, 9, 3, 4, 5, 3, 9, 8, 2, 4};
int cylinderSymbols2[22] = {2, 4, 2, 1, 4, 5, 6, 9, 7, 4, 1, 4, 5, 1, 3, 9, 0, 2, 4, 8, 0, 8};
int cylinderSymbols3[22] = {4, 1, 4, 3, 0, 9, 6, 0, 9, 4, 9, 6, 3, 8, 2, 4, 1, 0, 4, 5, 3, 2};

// starting picture of cylinder
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
	//int shiftStart[18] = {75,74,73,70,66,62,56,50,44,38,31,25,19,13,9,5,2,1}; // slow start array. Like fade in
	int shiftStart[18] = {50,50,48,47,44,41,38,34,29,25,21,16,13,9,6,3,2,1};
	int shiftStartLength = 18; // length of shiftStart array. the slowing array.
	int shiftStartPosition = 0; // position for slow start

	int *rollingArray = {nullptr};
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
	 * @param {int} _arrSize size of the cylinderArr array
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

	/**
	 * Returns if cylinder is rolling
	 */
	bool isCylinderRolling() {
		return isRolling;
	}

	/**
	 * Generates shifting array for the roll
	 */
	void generateShiftArray(int _count) {
		int count = _count * 9;
		rollingArrayLength = count;
		int *myArray;

		myArray = new int[count];

		for (int i = 0; i < count; i++) {
			// makes the starting speed slower
			if (shiftStartPosition < shiftStartLength) {
				myArray[i] = shiftStart[shiftStartPosition];
			}	else {
				myArray[i] = shiftSpeed;
			}
			shiftStartPosition++;
		}
		shiftStartPosition = 0; // reset position

		rollingArray = myArray;
		delete [] myArray;
	}

	/**
	 * Updating function which rolls the symbol
	 */
	void update() {
		int updDelay;
		if (isRolling == true) { // only if its rolling

			updDelay = *(rollingArray + rollingArrayIndex);
			if (updDelay < 0 || (updDelay > *(rollingArray + 1))) {
				updDelay = *(rollingArray + (rollingArrayIndex - 1));
			}

			rollDelay.start(updDelay); // starts the roll
			if (rollDelay.elapsed()) {

				Serial.print("TICK ");
				Serial.print(*(rollingArray + rollingArrayIndex));
				Serial.print(" -- ");
				Serial.print(rollingArrayIndex);
				Serial.print(" of ");
				Serial.print(rollingArrayLength);
				Serial.print(" | delay: ");
				Serial.println(updDelay);


				// rolling has finished
				if ((rollingArrayLength-1) < rollingArrayIndex) {
					isRolling = false;
					rollingArrayIndex = 0;

					EasyBuzzer.stopBeep();
					EasyBuzzer.singleBeep(131, 30);

					Serial.println("!!!! Finished rolling...");
				} else {
					if (rollingArrayIndex % 9 == 0) { // time to print symbol
						//Serial.println("### SYMBOL ###");
						if (realIndex >= arrSize) { // endless shift thru the cylinder array
							realIndex = realIndex - arrSize;
						}
						// display symbol to hidden place
						memcpy_P(buffer, CH + 10 * p_cylinderArr[realIndex], 10);
						p_dotMatrix->writeSprite(9, 0, buffer); // writes to 9th position because of 1px space
						p_dotMatrix->setColumn(9 + buffer[0], 0);

						realIndex++;
					}

					p_dotMatrix->shiftLeft(false, false); // shift one line
					rollingArrayIndex++;
				}
			}
		}
	}

	/**
	 * Start rolling the cylinder
	 */
	void roll() {
		isRolling = true;
	}

	/**
	 * Returns the symbol value in cylinder
	 */
	int getPosition() {
		return p_cylinderArr[realIndex - 1];
	}


};

/**
 * Flashes or permanent turns on/off the LED connected to a pin
 */
class Flasher {
	int ledPin;      // the number of the LED pin
	long OnTime;     // milliseconds of on-time
	long OffTime;    // milliseconds of off-time
	int ledState;    // ledState used to set the LED
	unsigned long previousMillis;  	// will store last time LED was updated
	bool updateFlshr = false; // global on/off

  public:
  Flasher(int pin, long on, long off) {
		ledPin = pin;
		pinMode(ledPin, OUTPUT);

		OnTime = on;
		OffTime = off;

		ledState = LOW;
		previousMillis = 0;
  }

  void update() {
		if (updateFlshr == true) {
			// check to see if it's time to change the state of the LED
			unsigned long currentMillis = millis();

			if ((ledState == HIGH) && (currentMillis - previousMillis >= OnTime)) {
				ledState = LOW;  // Turn it off
				previousMillis = currentMillis;  // Remember the time
				digitalWrite(ledPin, ledState);  // Update the actual LED
			}
			else if ((ledState == LOW) && (currentMillis - previousMillis >= OffTime)) {
				ledState = HIGH;  // turn it on
				previousMillis = currentMillis;   // Remember the time
				digitalWrite(ledPin, ledState);	  // Update the actual LED
			}
		}
  }

	void setInterval(long on, long off) {
		OnTime = on;
		OffTime = off;
	}

	void blinkOn() {
		updateFlshr = true;
	}

	void permanentOn() {
		updateFlshr = false;
		digitalWrite(ledPin, HIGH);
	}

	void off() {
		updateFlshr = false;
		ledState = LOW;
		digitalWrite(ledPin, LOW);
	}
};

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// initialize LCD on I2C pins
LiquidCrystal_I2C lcd(0x27,16,4);

// initialize all cylinders
SlotCylinder cylinder1(cylinderSymbols1, 22, 15);
SlotCylinder cylinder2(cylinderSymbols2, 22, 15);
SlotCylinder cylinder3(cylinderSymbols3, 22, 15);

// initialize buttons
OneButton startButton(BUTTON_START, true); // Buttons can be on analog pins
Flasher   startLed(LED_START, 300, 300); // set light to current button
OneButton betButton(BUTTON_BET, true);

OneButton coinInserter(COIN_INSERTER, true);

// initialize main variables
bool slotRunning = false;
bool winner = false;
VirtualDelay lcdDelay;

// songs definitons
const char songWinner[] PROGMEM = "winner:d=32:b=130:f5,c6,f5,c6,f5,c6,f5,c6,16f6";
const char songGameOver[] PROGMEM = "gamovr:d=8:b=200:e5,b4,g4,8p,2e4";
const char songInsertCoin[] PROGMEM = "coin:d=16:b=140:e5,8e6";
ProgmemPlayer player(BUZZER);

// credit and bets related stuff
int credit = 0;

int bet = 1; // value of bet
int betsArr[13] = {1, 2, 5, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100}; // possible bets
int betPos = 0; // position of bet in bet array
int betLen = 13; // length of bet array


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/**
 * Prints label and value to LCD. Lines are defined from 0 to 1
 */
void printNumberWithLabelToLCD(char *label, int value, int valueStartingPosition, int line = 0) {
	char buffer[50];

	// i know this is messy piece of s**ty code, deal with it B-). I don't wanna talk about it!!!
	if (value >= 0 && value < 10) {
		snprintf(buffer, 50, "%s%u     ", label, value);
	} else if (value >= 10 && value < 100) {
		snprintf(buffer, 50, "%s%u    ", label, value);
	} else if (value >= 100 && value < 1000) {
		snprintf(buffer, 50, "%s%u   ", label, value);
	} else if (value >= 1000 && value < 10000) {
		snprintf(buffer, 50, "%s%u  ", label, value);
	} else if (value >= 10000 && value < 100000) {
		snprintf(buffer, 50, "%s%u ", label, value);
	} else if (value >= 100000 && value < 1000000) {
		snprintf(buffer, 50, "%s%u", label, value);
	}

	lcd.setCursor(0, line);
	lcd.print(buffer);
}

void playWinSong() {
	EasyBuzzer.stopBeep(); // stops any beeps
	player.setSong(songWinner);
	player.finishSong(); // plays win song
}

void playGameOverSong() {
	EasyBuzzer.stopBeep(); // stops any beeps
	player.setSong(songGameOver);
	player.finishSong(); // plays gameover song
}

void playInsertCoinSong() {
	EasyBuzzer.stopBeep(); // stops any beeps
	player.setSong(songInsertCoin);
	player.finishSong(); // plays gameover song
}

void updateBetInfo() {
	lcd.setCursor(11, 0);
	lcd.print("     ");
	printNumberWithLabelToLCD("Bet: ", bet, 5, 0); // update bet info
}

/**
 * Push the start button
 */
void startButtonFn() {
	if (credit >= bet && slotRunning == false) { // start only if player have credit and reels dont rotate
		credit -= bet; // bets the money

		startLed.off(); // turns off the light on button

		updateBetInfo();
		printNumberWithLabelToLCD("Credit: ", credit, 8, 1); // update credit info

		randomSeed(analogRead(A0)); // A0 must be free (dunno, maybe :))
		int firstNumber = random(10, 25);
		int secondNumber = random(10);
		int thirdNumber = random(10);

		secondNumber = firstNumber + secondNumber;
		thirdNumber = secondNumber + thirdNumber;
		if (firstNumber == secondNumber) {
			secondNumber++;
		}
		if (secondNumber == thirdNumber) {
			thirdNumber++;
		}

		Serial.println("RND: ");
		Serial.println(firstNumber);
		Serial.println(secondNumber);
		Serial.println(thirdNumber);

		cylinder1.generateShiftArray(15);
		cylinder2.generateShiftArray(secondNumber);
		cylinder3.generateShiftArray(thirdNumber);
		cylinder1.roll();
		cylinder2.roll();
		cylinder3.roll();

		slotRunning = true;
	} else {
		// make deep beep sound or something error-like
	}
}

void betButtonFn() {
	if (slotRunning == false) { // only allowed to push when cylinders are not turning
		betPos++;
		if (betPos == betLen) { // scroll thru bet array
			betPos = 0;
		}
		bet = betsArr[betPos];

		updateBetInfo();

		EasyBuzzer.stopBeep();
		EasyBuzzer.singleBeep(262, 40);
	}
}

void coinInserterFn() {
	if (slotRunning == false) {
		credit = credit + 20;
		updateBetInfo();
		printNumberWithLabelToLCD("Credit: ", credit, 8, 1); // update credit info
		playInsertCoinSong();
	}
}

/**
 * Watcher for the slot machine. Resolves, if you win or lose.
 */
void slotWatch() {
	if (slotRunning == true) {
		// cylinders are rolling

		if (cylinder1.isCylinderRolling() == false && cylinder2.isCylinderRolling() == false && cylinder3.isCylinderRolling() == false) {
			// all cylinders stopped

			Serial.println("###############");
			Serial.print("CYL 1: ");
			Serial.println(cylinder1.getPosition());
			Serial.print("CYL 2: ");
			Serial.println(cylinder2.getPosition());
			Serial.print("CYL 3: ");
			Serial.println(cylinder3.getPosition());
			Serial.println("---------------");
			/* TODO
				- first two symbols pays as 0.5 bet MAYBE
			*/

			// all symbols are the same
			if (winner == false &&
					 (cylinder1.getPosition() == cylinder2.getPosition() &&
			      cylinder2.getPosition() == cylinder3.getPosition())
				) {
				// WIN
				playWinSong();
				credit += (symbolValue[cylinder1.getPosition()] * bet);
				winner = true;
				Serial.println("*** WINNER ***");
			}

			/* JOKERS: If bet > 10, joker symbol (9) act as any symbol.
			   x | J | x
				 x | x | J
				 x | J | J
			*/
			if (winner == false && (bet >= 10 &&
					((cylinder2.getPosition() == 9 && cylinder1.getPosition() == cylinder3.getPosition()) ||
					(cylinder3.getPosition() == 9 && cylinder1.getPosition() == cylinder2.getPosition()) ||
					(cylinder2.getPosition() == 9 && cylinder3.getPosition() == 9)
			))) {
				// WIN
				playWinSong();
				credit += (symbolValue[cylinder1.getPosition()] * bet);
				winner = true;
				Serial.println("*** WINNER with JOKERS ***");
			}
			// J | x | J
			if (winner == false && (bet >= 10 &&
					((cylinder1.getPosition() == 9 && cylinder3.getPosition() == 9)
			))) {
				// WIN
				playWinSong();
				credit += (symbolValue[cylinder2.getPosition()] * bet);
				winner = true;
				Serial.println("*** WINNER with JOKERS ***");
			}
			/*
				 J | J | x
				 J | x | x
			*/
			if (winner == false && (bet >= 10 &&
					((cylinder1.getPosition() == 9 && cylinder2.getPosition() == cylinder3.getPosition()) ||
					(cylinder1.getPosition() == 9 && cylinder2.getPosition() == 9)
			))) {
				// WIN
				playWinSong();
				credit += (symbolValue[cylinder3.getPosition()] * bet);
				winner = true;
				Serial.println("*** WINNER with JOKERS ***");
			}
			// JOKERS end

			if (winner == false) {
				// LOSE
				if (credit <= 0) {
					playGameOverSong();
					credit = 0;
					lcd.setCursor(0, 1);
					lcd.print("  iNSERT  C0iN  ");
					startLed.off();
					Serial.println("!!! GAME OVER !!!");
				} else {
					Serial.println("-_- LOSER -_-");
				}

				Serial.print("credit: ");
				Serial.println(credit);
			}

			if (credit > 0) {
				printNumberWithLabelToLCD("Credit: ", credit, 8, 1); // update credit info
				startLed.blinkOn();
			}

			slotRunning = false;
			winner = false;
		}
	}
}

// =======================================================================

void setup() {
    Serial.begin(115200);

		// init LCD
		lcd.init();
		lcd.backlight();
  	lcd.setCursor(0, 0);
  	lcd.print("  SL0T MACHiNE  ");
		lcd.setCursor(0, 1);
  	lcd.print("  iNSERT  C0iN  ");

		// init display 8x8 matrix with DIN, CS, CLK
		cylinder1.initMatrix(CYLINDER_1_DIN, CYLINDER_1_CS, CYLINDER_1_CLK);
		cylinder2.initMatrix(CYLINDER_2_DIN, CYLINDER_2_CS, CYLINDER_2_CLK);
		cylinder3.initMatrix(CYLINDER_3_DIN, CYLINDER_3_CS, CYLINDER_3_CLK);

		// attach button to function
		startButton.attachClick(startButtonFn);
		betButton.attachClick(betButtonFn);

		// attach coin inserter
		coinInserter.attachClick(coinInserterFn);

		// initialize easybuzzer
		EasyBuzzer.setPin(BUZZER);

		// blink start led
		startLed.blinkOn();
}

void loop() {

		// button watcher
		startButton.tick();
		betButton.tick();
		coinInserter.tick();

		// cylinder watcher
		cylinder1.update();
		cylinder2.update();
		cylinder3.update();

		// slot game winning/losing watcher
		slotWatch();

		// LED watcher
		startLed.update();

		// sound watcher
		EasyBuzzer.update();

}