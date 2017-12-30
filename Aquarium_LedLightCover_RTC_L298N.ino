#include <DS3231.h>

// Init the DS3231 using the hardware interface
DS3231  rtc(SDA, SCL);

// Init a Time-data structures
Time  currentTime; //time from RTC
Time t; // Time variable for operations
Time lightPauseTimer; // Time variable for operations
#define NUMBER_OF_WHITE_LED_BARS_ON_RELAY 4 //How many white led bars(connected to relay) to use with PWN white led bat
#define NUMBER_OF_WHITE_LED_BARS 5


#define LED_RELAY_1 2 //Relay on the relay shield with white LED bar
#define LED_RELAY_2 3 //Relay on the relay shield with white LED bar
#define LED_RELAY_3 4 //Relay on the relay shield with white LED bar 
#define LED_RELAY_4 5 //Relay on the relay shield with white LED bar
#define WHITE_LED_PWM 6 //L298N PWM output #1 for white LED bar
#define BLUE_LED_PWM 10 //L298N PWM output #3 for blue LED bar
#define CIAN_LED_PWM 11 //L298N PWM output #4 for white LED bar
#define BUTTON 12 //Button to make instant dark fo 2 hours

#define LIGHT_PAUSE_HOURS 2 //Button to make instant dark fo 2 hours


#define sunriseHour 7 //Hour for sunrise (0->255)
#define dayHour 10 // Hour for day(255) start
#define sunsetHour 17 // Hour for sunset(255->0) start
#define nightHour 20 //Hour for night(0)



int sunriseDurationInSeconds;
int sunsetDurationInSeconds;
int sunriseStepDurationInSeconds;
int sunsetStepDurationInSeconds;
int oneMoreWhiteLedBarOnPeriodInSeconds;
int oneMoreWhiteLedBarOffPeriodInSeconds;
long lastLightSet = 0;
long ligtPauseSet = 0;
bool lightPause = false;

//button debounce

int buttonState = HIGH;             // the current reading from the input pin
int lastButtonState = HIGH;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 100;    // the debounce time; increase if the output flickers

void setup()
{
	// Setup Serial connection
	Serial.begin(115200);

	// Initialize the rtc object
	rtc.begin();
	lightPauseTimer = rtc.getTime();
	currentTime = rtc.getTime();

	//The following lines can be uncommented to set the date and time

	//rtc.setTime(21, 20, 00);     // Set the time to 21:58:00 (24hr format)

	// t =  00:00:00 on January 1th 2017 - For timespan culculations
	t.hour = 0;
	t.min = 0;
	t.sec = 0;
	t.year = 2017;
	t.mon = 1;
	t.date = 1;

	sunriseDurationInSeconds = (dayHour - sunriseHour) * 60 * 60;
	sunsetDurationInSeconds = (nightHour - sunsetHour) * 60 * 60;

	sunriseStepDurationInSeconds = sunriseDurationInSeconds / 255;
	sunsetStepDurationInSeconds = sunsetDurationInSeconds / 255;

	oneMoreWhiteLedBarOnPeriodInSeconds = sunriseDurationInSeconds / (NUMBER_OF_WHITE_LED_BARS); // So after ... seconds we enable one more led bar connected to relay shield
	oneMoreWhiteLedBarOffPeriodInSeconds = sunsetDurationInSeconds / (NUMBER_OF_WHITE_LED_BARS);
	
	pinMode(LED_RELAY_1, OUTPUT);
	pinMode(LED_RELAY_2, OUTPUT);
	pinMode(LED_RELAY_3, OUTPUT);
	pinMode(LED_RELAY_4, OUTPUT);
	pinMode(WHITE_LED_PWM, OUTPUT);
	pinMode(BLUE_LED_PWM, OUTPUT);
	pinMode(CIAN_LED_PWM, OUTPUT);
	analogWrite(WHITE_LED_PWM, 0);
	digitalWrite(LED_RELAY_1, HIGH);
	digitalWrite(LED_RELAY_2, HIGH);
	digitalWrite(LED_RELAY_3, HIGH);
	digitalWrite(LED_RELAY_4, HIGH);
	pinMode(BUTTON, INPUT);           // set pin to input
	digitalWrite(BUTTON, HIGH);       // turn on pullup resistors

}

void loop() {
	// Get data from the DS3231 and set up for further culculations
	currentTime = rtc.getTime();
	currentTime.year = 2017;
	currentTime.mon = 1;
	currentTime.date = 1;
	long now = millis();
	if (now - lastLightSet > 5000 && !lightPause) {
		lastLightSet = now;
		Serial.println(rtc.getTimeStr());
		setLightLevel(currentTime);
	}
	readButton();
	if (buttonState == LOW && currentTime.hour <= nightHour) // add two hours works before 21.00
	{
		lightPauseTimer = rtc.getTime();
		lightPauseTimer.year = 2017;
		lightPauseTimer.mon = 1;
		lightPauseTimer.date = 1;
		lightPauseTimer.hour += LIGHT_PAUSE_HOURS;
		lightPause = true;
		whiteLedBarsLightLevel(0);
		analogWrite(WHITE_LED_PWM, 0);
		Serial.println("Light pause activated");
		Serial.print("Light will ON ");
		Serial.print(lightPauseTimer.hour);
		Serial.print(":");
		Serial.println(lightPauseTimer.min);
		delay(2000);
	}
	long utCurrent = rtc.getUnixTime(currentTime);
	long utFromTimer = rtc.getUnixTime(lightPauseTimer);
	if (now - ligtPauseSet > 5000 && lightPause) {
		ligtPauseSet = now;
		long countDown = (utFromTimer - utCurrent) / 60;
		Serial.print("Light will in ");
		Serial.print(countDown);
		Serial.println(" minutes");
		if (utCurrent > utFromTimer)
		{
			lightPause = false;
		}
	}

}

void readButton() {
	int reading = digitalRead(BUTTON); //When pressed is LOW
	// check to see if you just pressed the button
	// (i.e. the input went from HIGH to LOW), and you've waited long enough
	// since the last press to ignore any noise:

	// If the switch changed, due to noise or pressing:
	if (reading != lastButtonState) {
		// reset the debouncing timer
		lastDebounceTime = millis();
	}
	if ((millis() - lastDebounceTime) > debounceDelay) {
		// whatever the reading is at, it's been there for longer than the debounce
		// delay, so take it as the actual current state:

		// if the button state has changed:
		if (reading != buttonState) {
			buttonState = reading;			
		}
	}
	// save the reading. Next time through the loop, it'll be the lastButtonState:
	lastButtonState = reading;
}


void setLightLevel(Time currentTime) {
	//--- sunrise block---
	if (currentTime.hour >= sunriseHour && currentTime.hour < dayHour) {
		t.hour = sunriseHour;
		long time1 = rtc.getUnixTime(t);
		long time2 = rtc.getUnixTime(currentTime);
		long secondsPassedFromSunrise = time2 - time1;
		int lightLevel = (secondsPassedFromSunrise) / sunriseStepDurationInSeconds;
		Serial.print("It is sunrise time now, light level: ");
		Serial.println(lightLevel);
		analogWrite(BLUE_LED_PWM, lightLevel);
		analogWrite(CIAN_LED_PWM, lightLevel);
		int numberOfLedBar = (int)secondsPassedFromSunrise / oneMoreWhiteLedBarOnPeriodInSeconds;
		whiteLedBarsLightLevel(numberOfLedBar);
		float lightLevelFloat = (secondsPassedFromSunrise % oneMoreWhiteLedBarOnPeriodInSeconds) / sunriseStepDurationInSeconds;
		lightLevel = (int)lightLevelFloat;
		analogWrite(WHITE_LED_PWM, lightLevel);
	};
	if (currentTime.hour >= dayHour && currentTime.hour < sunsetHour) {
		t.hour = 0;
		Serial.println("It is day time now, light level: 255");
		analogWrite(BLUE_LED_PWM, 255);
		analogWrite(CIAN_LED_PWM, 255);
		analogWrite(WHITE_LED_PWM, 255);
		whiteLedBarsLightLevel(NUMBER_OF_WHITE_LED_BARS_ON_RELAY);
	};
	if (currentTime.hour >= sunsetHour && currentTime.hour < nightHour) {
		t.hour = sunsetHour;
		long time1 = rtc.getUnixTime(t);
		long time2 = rtc.getUnixTime(currentTime);
		int lightLevel = 255 - ((time2 - time1) / sunsetStepDurationInSeconds);
		Serial.print("It is sunset time now, light level: ");
		Serial.println(lightLevel);
		analogWrite(BLUE_LED_PWM, lightLevel);
		analogWrite(CIAN_LED_PWM, lightLevel);
		long secondsPassedFromSunset = time2 - time1;
		int numberOfLedBarToDisable = (int)secondsPassedFromSunset / oneMoreWhiteLedBarOffPeriodInSeconds;
		whiteLedBarsLightLevel(NUMBER_OF_WHITE_LED_BARS_ON_RELAY - numberOfLedBarToDisable);
		lightLevel = (int)(secondsPassedFromSunset % oneMoreWhiteLedBarOffPeriodInSeconds) / sunsetStepDurationInSeconds;
		Serial.print("lightLevel on WHITE_LED_PWM to deduct from 255: ");
		Serial.println(lightLevel);
		analogWrite(WHITE_LED_PWM, 255 - lightLevel);
	}
	
	// Night befor 24 h || Night after 0 h
	if (currentTime.hour >= nightHour || currentTime.hour < sunriseHour) {
		t.hour = 0;
		Serial.println("It is night time now, light level: 0");
		analogWrite(BLUE_LED_PWM, 0);
		analogWrite(CIAN_LED_PWM, 0);
		analogWrite(WHITE_LED_PWM, 0);
		whiteLedBarsLightLevel(0);
	};
};



void whiteLedBarsLightLevel(int ledBarsNumber) {
	switch (ledBarsNumber) {
	case 0:
		digitalWrite(LED_RELAY_1, HIGH);
		digitalWrite(LED_RELAY_2, HIGH);
		digitalWrite(LED_RELAY_3, HIGH);
		digitalWrite(LED_RELAY_4, HIGH);
		break;
	case 1:
		digitalWrite(LED_RELAY_1, LOW);
		digitalWrite(LED_RELAY_2, HIGH);
		digitalWrite(LED_RELAY_3, HIGH);
		digitalWrite(LED_RELAY_4, HIGH);
		break;
	case 2:
		digitalWrite(LED_RELAY_1, LOW);
		digitalWrite(LED_RELAY_2, LOW);
		digitalWrite(LED_RELAY_3, HIGH);
		digitalWrite(LED_RELAY_4, HIGH);
		break;
	case 3:
		digitalWrite(LED_RELAY_1, LOW);
		digitalWrite(LED_RELAY_2, LOW);
		digitalWrite(LED_RELAY_3, LOW);
		digitalWrite(LED_RELAY_4, HIGH);
		break;
	case 4:
		digitalWrite(LED_RELAY_1, LOW);
		digitalWrite(LED_RELAY_2, LOW);
		digitalWrite(LED_RELAY_3, LOW);
		digitalWrite(LED_RELAY_4, LOW);
		break;
	default:
		digitalWrite(LED_RELAY_1, HIGH);
		digitalWrite(LED_RELAY_2, HIGH);
		digitalWrite(LED_RELAY_3, HIGH);
		digitalWrite(LED_RELAY_4, HIGH);
		break;
	}
};