/* 
Demonstration code for the Parallax PAM-7Q module, #28509
This code uses the default factory settings of the PAM-7Q module.
The GPS output is displayed in the Arduino Serial Terminal Window.
After uploading the sketch open this window to view the output. 
Make sure the baud rate is set to 9600.
Numeric output is shown as signed latitude and longitude degrees and
minutes. Values may be directly copied and pasted into the location bar
of Google Maps to visually show your location.
For best results use the PAM-7Q module outdoors, or near an open window.
Use indoors away from windows may result in inconsistent results.
This example code is for the Arduino Uno and direct compatible boards. 
It has not been tested, nor designed for, other Arduino boards, including
the Arduino Due.
Important: This version is intended for Arduino 1.0 or later IDE. It will
not compile in earlier versions. Be sure the following files are
present in the folder with this sketch:
TinyGPS.h
TinyGPS.cpp
keywords.txt
A revised version of the TinyGPS object library is included in the sketch folder
to avoid conflict with any earlier version you may have in the Arduino libraries 
location.
Connections:
PAM-7Q    Arduino
GND       GND
VDD       5V
TXD       Digital Pin 6
 
Reminder! Wait for the satellite lock LED to begin flashing before
taking readings. The flashing LED indicates satellite lock. Readings taken
before satellite lock may be inaccurate.
*/

// for GPS
#include <math.h>
#include <SoftwareSerial.h>
#include "./TinyGPS.h"                  // Use local version of this library
TinyGPS gps;
// for Sensor
#include <Wire.h>
//#define ADDRESS 0x76 //0x77

// for GPS------------------------------------------------------------------------------------------
unsigned long age;
//float balloonLat              = 40;//34.28889;//[degrees]
//float balloonLon              = 50;//91.6458;//[degrees]
//float balloonAlt              = 60;//10064.0;///5280.0;//[miles]
long longBalloonLat             = 10;//34.28889;//[degrees] /////////////////////////////////////////////////////FIX THIS lolo
long longBalloonLon             = 20;//91.6458;//[degrees]
long longBalloonAlt             = 30;//10064.0;///5280.0;//[miles]
unsigned long longBalloonTime   = 40;
unsigned long longBalloonDate   = 50;


// for Housekeeping------------------------------------------------------------------------------------------
unsigned char Vcount;
unsigned char Icount;

//const byte rxPin = 0;
//const byte txPin = 1;

// for Sensor------------------------------------------------------------------------------------------
uint32_t D1 = 0;
uint32_t D2 = 0;
int64_t dT = 0;
int32_t TEMP = 0;
int64_t OFF = 0; 
int64_t SENS = 0; 
int32_t P = 0;
uint16_t C[7];
unsigned int Temperature_cC; //RealTemp +60C (to remove negative) then *10^2 to get temp in centiCelsius. 
unsigned long Pressure_dP; //in decaPascels *10^2;

//For sending
char endLine[3] = {'E', 'N', 'D'};
//char CharsToSend[24];
//char* CharsToSend = malloc(24);
unsigned char* CharsToSend;// = "HELLO!!!!!!!!!!!!!!!!!!!!!!!!!!!";// malloc(32);
unsigned char* writeTo=CharsToSend;
unsigned char* writeArray=CharsToSend;
unsigned char** wrPtr=&writeArray;

bool newdata = false;
unsigned short LineLength = 29; //excludes checksum byte
int64_t start;

void setup() {
	// for communication with Pi
	Wire.begin(4);

	Serial.begin(74880);
	
	//for GPS------------------------------------------------------------------------------------------
	Serial1.begin(9600);                     // Communicate at 9600 baud (default for PAM-7Q module)
	
	
	delay(200);

	// for Sensor------------------------------------------------------------------------------------------
	// Disable internal pullups, 10Kohms are on the breakout
	PORTC |= (1 << 4);
	PORTC |= (1 << 5);
	delay(100);

	//initial(ADDRESS);
	//GPSstuff(); 
	//SENSORstuff();
	//updateCharsToSend();

	Wire.onRequest(requestEvent); // register event

	updateCharsToSend();
	
	start = millis();       // starts a count of millisec since the code began  
}

void loop() {
	
	//Serial.print((char) CharsToSend[0]);
	//Serial.print((char) CharsToSend[1]);
	//Serial.print((char) CharsToSend[2]);
	//Serial.print((char) CharsToSend[3]);
	//Serial.print((char) CharsToSend[4]);
	//Serial.print((char) CharsToSend[5]);

	//delay(1000);
	GPSstuff();
	houseKeeping();
	updateCharsToSend();

  
  //if(newdata){
    
	  // longBalloonLat = (unsigned long) (longBalloonLat*100000);
	  // longlongBalloonLon = (unsigned long) (longBalloonLon*100000);
	  // longBalloonAlt = (unsigned long) (longBalloonAlt*100);
	  // longBalloonTime = (unsigned long) (balloonTime*100);
	  	
	  
	  
	//  writeArray=CharsToSend;
	//  wrPtr=&writeArray;

	//  Serial.println((unsigned long)getIntFromByte(wrPtr,3));
	//
	//  Serial.println((unsigned long)getIntFromByte(wrPtr,4));
	//  
	//  Serial.println((unsigned long)getIntFromByte(wrPtr,4));
	//
	//  Serial.println((unsigned long)getIntFromByte(wrPtr,3));
	//
	//  Serial.println((unsigned int)getIntFromByte(wrPtr,2));
	//  
	//  Serial.println((unsigned long)getIntFromByte(wrPtr,3));
	//
	//  Serial.println((char)getIntFromByte(wrPtr,1));
	//
	//  Serial.println((char)getIntFromByte(wrPtr,1));
	//
	//  Serial.println((char)getIntFromByte(wrPtr,1));
	  
	  //lineCount; //Wierd. This must be here for linecount to increment in the requestEvent()
	  //lineCount++; // increments in updateCharsToSend
  //}
  


	Serial.print("LAT: ");
	Serial.println(longBalloonLat);
	Serial.print("LON: ");
	Serial.println(longBalloonLon);
	Serial.print("ALT: ");
	Serial.println(longBalloonAlt);
	Serial.print("DATE: ");
	Serial.println(longBalloonDate);
	Serial.print("TIME: ");
	Serial.println(longBalloonTime);
	Serial.println("");
  
	delay(500);
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
	Serial.println("requestEvent");
	//GPSstuff();
	//SENSORstuff();
	//char* CharsToSend = updateCharsToSend();
	//updateCharsToSend();
	//lineCount++;
	//updateCharsToSend();

	Wire.write(CharsToSend, 30); // respond with message of 32 byte
	//Wire.write("W", 1); // respond with message of 32 byte

	//updateCharsToSend();
	//free(CharsToSend);
	//Wire.write("ftgyho04856000r57j0k?0");

	//GPSstuff();
	//SENSORstuff();
  
  
}

//char* updateCharsToSend(){
void updateCharsToSend(){
	free(CharsToSend);
	CharsToSend = malloc(LineLength+1);
	writeTo=CharsToSend;
	unsigned int sum = 0;
	short i;

	//Line counter-------------------------------------------
	unsigned int intBuflineCount = 4567;
	insertBytesFromInt(&intBuflineCount, &writeTo, 2);

	//Latitude * 10^5 positive only----------------should be 10^10-----------
	//longBuflatitude = (unsigned long long)(longBalloonLat * 100000);
	insertBytesFromInt(&longBalloonLat, &writeTo, 3);

	//Longitude * 10^5 positive only max of 109 degrees---should be 10^10-----
	//longBuflongitude = (unsigned long long)(longBalloonLon * 100000);
	insertBytesFromInt(&longBalloonLon, &writeTo, 3);

	//Altitude * 100--------------------------------------------
	//long intBufaltitude = 1000 * 100;
	insertBytesFromInt(&longBalloonAlt, &writeTo, 3);
	
	//Time (seconds since UTC half day) --------------------------------------------
	unsigned int longBalloonTime = 450;
	insertBytesFromInt(&longBalloonTime, &writeTo, 2);

	//Thermistor count------------------------------------------
	unsigned int intBuftemperature = 450;
	insertBytesFromInt(&intBuftemperature, &writeTo, 2);

	//Battery Voltage---------------------------------------------
	//unsigned short intBufpressure = 120;
	insertBytesFromInt(&Vcount, &writeTo, 1);
	
	//Battery Current---------------------------------------------
	//unsigned short intBufpressure12 = 140;
	insertBytesFromInt(&Icount, &writeTo, 1);

	//Magnotometer X---------------------------------------------
	unsigned short intBufpressure2 = 80;
	insertBytesFromInt(&intBufpressure2, &writeTo, 1);

	//Magnotometer Y---------------------------------------------
	unsigned short intBufpressure3 = 60;
	insertBytesFromInt(&intBufpressure3, &writeTo, 1);

	//Magnotometer Z---------------------------------------------
	unsigned short intBufpressure4 = 40;
	insertBytesFromInt(&intBufpressure4, &writeTo, 1);

	//Humidity---------------------------------------------
	unsigned short intBufpressure5 = 96;
	insertBytesFromInt(&intBufpressure5, &writeTo, 1);

	//Pressure---------------------------------------------
	unsigned long intBufpressure6 = 102300;
	insertBytesFromInt(&intBufpressure6, &writeTo, 3);

	//Internal Temperature---------------------------------------------
	unsigned int intBufpressure7 = 15;
	insertBytesFromInt(&intBufpressure7, &writeTo, 2);

	CharsToSend[LineLength-3] = endLine[0];
	CharsToSend[LineLength-2] = endLine[1];
	CharsToSend[LineLength-1] = endLine[2];
	
	for(i=0;i<LineLength;i++){
		sum += (unsigned char)CharsToSend[i];
	}
	CharsToSend[LineLength] = (sum%64);
}


void insertBytesFromInt(void* value,unsigned char** byteStart, short numberBytesToCopy){

	unsigned char* valueBytes=value;
	short loopCount=0;
	for(loopCount=0;loopCount<numberBytesToCopy;loopCount++){
	(*byteStart)[loopCount]=valueBytes[loopCount];
	}
	*byteStart+=(short)numberBytesToCopy;
}

unsigned long getIntFromByte(unsigned char** arrayStart, short bytes){
//unsigned long long getIntFromByte(unsigned char** arrayStart, short bytes){

	//Allocating array to read into
	unsigned char* intPtr=malloc (sizeof(unsigned long long));
	unsigned long long temp;
	//Void pointer to same location to return

	//Loop Counter
	short loopCount;
	for(loopCount=0;loopCount<sizeof(unsigned long);loopCount++){

		//Copying bytes from one array to the other
		if(loopCount<bytes){
		  intPtr[loopCount]=(*arrayStart)[loopCount];
		}else{
            intPtr[loopCount]=0;
        }
	}
	*arrayStart+=(short)bytes;
	temp=*((unsigned long long*)intPtr);
	free(intPtr);
	//Returning void pointer (Pointer to an integer with the designated of the number of bytes)
	return temp;
}

void GPSstuff() {
	newdata = false;
	int64_t hack = millis();
	if (hack - start > 250) {     // Update every 1 seconds
		start = hack;
		if (feedgps()){                    // if serial1 is available and can read gps.encode
			newdata = true;
		}
	}
	if (newdata) {  // if locked
		gpsdump(gps);
		 

		//using GPS ------------------------------------------------------------------------------------------
		Serial.println("LOCKED ON");
		//Serial.print("Balloon Altitude: ");
		
		
	}else{          // if not locked
		Serial.println("Not Locked");
	}
}

// Get and process GPS data
void gpsdump(TinyGPS &gps) {
	longBalloonAlt = gps.altitude();
	gps.get_position(&longBalloonLat, &longBalloonLon, &age);
  	longBalloonLon = -longBalloonLon;
	gps.get_datetime(&longBalloonDate, &longBalloonTime, &age);
	timeConvert(longBalloonTime);
}

// Feed data as it becomes available 
bool feedgps() {
	while (Serial1.available()) {
		if (gps.encode(Serial1.read()))
			return true;
	}
	return false;
}

// Feed data as it becomes available 
void houseKeeping() {
	int analogPinV = 0;     // potentiometer wiper (middle terminal) connected to analog pin 3outside leads to ground and +5V
	int analogPinI = 1; 
	double Vread = 0;           // variable to store the value read
	double Iread = 0;           // variable to store the value read

	Vread = round(analogRead(analogPinV)/4);    // read the input pin
	Iread = round(analogRead(analogPinI)/4);    // read the input pin
	double Vvoltread = (5*Vread)/255;    // read the input pin
	double Ivoltread = (5*Iread)/255;    // read the input pin
	double Vin = 3.2206*Vvoltread - 0.086;    // read the input pin
	double Iin = 0.1116*Ivoltread - 0.0009;    // read the input pin

	Serial.print("\nV Value: ");         
	Serial.print(Vread);      
	Serial.print(", ");       
	Serial.print(Vvoltread);   
	Serial.print(", ");       
	Serial.print(Vin);          
	Serial.print("\nI Value: ");    
	Serial.print(Iread);
	Serial.print(", ");
	Serial.print(Ivoltread);    
	Serial.print(", ");      
	Serial.print(Iin);         
	Serial.print("\n");   

	Vcount = (unsigned char) Vread;
	Icount = (unsigned char) Iread;
}

// Converts time from UTC hhmmsscc to the number of seconds since the last UTC half day (resets to 0 every 12 hours)
void timeConvert(unsigned long &timeVar){
	unsigned long buffVar = timeVar;
	unsigned short hours;
	unsigned short minutes;
	unsigned short seconds;
	
	buffVar = buffVar/100; // removes centi-seconds
	
	seconds = buffVar%100; // gets seconds
	buffVar = buffVar/100; // removes seconds
	
	minutes = buffVar%100; // gets minutes
	buffVar = buffVar/100; // removes minutes
	
	hours = buffVar%12; // only hours left, max of 24.
	
	timeVar = seconds + (60*minutes) + (3600*hours);
}
