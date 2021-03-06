/*

"~" in a commnet = ISSUE THAT NEEDS TO BE LOOKED AT


 
 SimpleSat Rotor Control Program  -  73 de W9KE Tom Doyle
 January 2012
 
 Written for Arduino 1.0
 
 This program was written for the Arduino boards. It has been tested on the
 Arduino UNO and Mega2560 boards. 
 
 Pin 7 on the Arduino is used as a serial tx line. It is connected to a Parallax 
 27977 2 X 16 backlit serial LCD display - 9600 baud. WWW.Parallax.com
 It is not required but highly recommended. You might want to order a
 805-00011 10-inch Extension Cable with 3-pin Header at the same time.
 The first row on the display will display the current rotor azimuth on
 the left hand side of the line. When the azimuth rotor is in motion 
 a L(eft) or R(ight) along with the new azimuth received from the tracking 
 program is displayed on the right side of the line. The second line will do
 the same thing for the elevation with a U(p) or D(own) indicating the 
 direction of motion.
 
 The Arduino usb port is set to 9600 baud and is used for receiving
 data from the tracking program in GS232 format.
 In SatPC32 set the rotor interface to Yaesu_GS-232.
 
 These pin assignments can be changed
 by changting the assignment statements below.
 G-5500 analog elevation to Arduino pin A0
 G-5500 analog azimuth to Arduino pin A1
 Use a small signal transistor switch or small reed relay for these connections
 G-5500 elevation rotor up to Arduino pin 8
 G-5500 elevation rotor down to Arduino pin 9
 G-5500 azimuth rotor left to Arduino pin 10
 G-5500 azimuth rotor right to Arduino pin 11
 
 The Arduino resets when a connection is established between the computer
 and the rotor controller. This is a characteristic of the board. It makes
 programming the chip easier. It is not a problem but is something you
 should be aware of.
 
 The program is set up for use with a Yaesu G-5500 rotor which has a max
 azimuth of 450 degrees and a max elevation of 180 degrees. The controller
 will accept headings within this range. If you wish to limit the rotation
 to 360 and/or limit the elevation to 90 set up SatPC32 to limit the rotation
 in the rotor setup menu. You should not have to change the rotor controller.
 
            - For additional information check -
 
      http://www.tomdoyle.org/SimpleSatRotorController/ 
*/

/* 
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
    INCLUDING BUT NOT LIMITED TO THE'WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
    PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 'COPYRIGHT HOLDERS BE 
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
    OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/ 

/*

COMMANDS:

COMMAND: 1 latitude longitude elevation
RETURN: (printed successful)
Sets the ground station's GPS location. It is usually only called once before tracking begins but has no limitation on number of calls. It is used in determining the pointing direction of the antenna during tracking. This command call has the integer command number and 3 double arguments. The arguments of latitude and longitude are in decimal degrees and elevation is in meters. Returns 1 if sucessful, -1 if not.

COMMAND: 2 latitude longitude elevation
RETURN: (printed successful)
Sets the tracking object's GPS location. This command it called with a high frequency but no more than 1 Hz. This command call has the integer command number and 3 double arguments. The arguments of latitude and longitude are in decimal degrees and elevation is in meters. Returns 1 if sucessful, -1 if not.

COMMAND: 3
RETURN: (printed azimuth elevation)
This command tells the tracking system to return the rotor's current azimuth and elevation. The values are then sent as a string of azimuth and elevation angles seperated by a space. The angles are in integer degrees.
*/

// ------------------------------------------------------------
// ---------- you may wish to adjust these values -------------
// ------------------------------------------------------------

// A/D converter parameters 
/*
   AFTER you have adjusted your G-5500 control box as per the manual
   adjust the next 4 parameters. The settings interact a bit so you may have
   to go back and forth a few times. Remember the G-5500 rotors are not all that
   accurate (within 4 degrees at best) so try not to get too compulsive when 
   making these adjustments. 
*/



#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "Wire.h"
#include <SoftwareSerial.h> // use software uart library

#define SLAVE_ADDRESS 0x04
int i;

const long _azAdZeroOffset   =   0;//325;//-3;   // adjust to zero out lcd az reading when control box az = 0 
const long _elAdZeroOffset   =   0;//7;   // adjust to zero out lcd el reading when control box el = 0

/*  
    10 bit A/D converters in the Arduino have a max value of 1023
    for the azimuth the A/D value of 1023 should correspond to 450 degrees
    for the elevation the A/D value of 1023 should correspond to 180 degrees
    integer math is used so the scale value is multiplied by 100 to maintain accuracy
    the scale factor should be 100 * (1023 / 450) for the azimuth
    the scale factor should be 100 * (1023 / 180) for the elevation    
*/

const long _azScaleFactor =  232;  //  adjust as needed
const long _elScaleFactor =  568;  //  adjust as needed 
/////
const long _dispazScaleFactor =  500;  //  adjust as needed
const long _dispelScaleFactor =  500;  //  adjust as needed 

// pins
const byte _elevationInputPin = A0; // elevation analog signal from G5500
const byte _azimuthInputPin = A1;   // azimuth analog signal from G5500
const byte _G5500UpPin = 9;        // elevation rotor up control line
const byte _G5500DownPin = 10;      // elevation rotor down control line
const byte _G5500LeftPin = 11;      // azimuth rotor left control line
const byte _G5500RightPin = 12;     // azimuth rotor right control line

// take care if you lower this value -  wear or dirt on the pots in your rotors
// or A/D converter jitter may cause hunting if the value is too low. 

long _closeEnough = 300;   // tolerance for az-el match in rotor move in degrees * 100
long _closeEnoughSmoothing = 500;   // if within this range (degree * 100), increase pausing between moves to limit current surges

//Values for calculations
const float Re = 6378;//[km] 
const float pi = 3.14159265359; 
float distancefromballoontoGS = 0;   
                                
float balloonLat              = 29.160626;//[degrees]
float balloonLon              = 80.114442;//[degrees]
float balloonAlt              = 35*100000;//[km](cm*100000)
//Hard coded for ERAU Daytona Beach Campus;//Test values
float groundStationlat        = 29.189106;//29.187941;//[degrees]
float groundStationlon        = 81.048949;//81.048325;//[degrees]
float groundStationAlt        = 0;//5710.0/5280.0;//[miles]29.187941, -81.048325
float d; 
float Azimuth; 
float Elevation; 
String command = "";

//                        1 latitude longitude elevation

//char* CharsToReceive = malloc(22);
//char* writeArray=CharsToReceive;
//char** wrPtr=&writeArray;

// ------------------------------------------------------------
// ------ values from here down should not need adjusting -----
// ------------------------------------------------------------

// rotor
const long _maxRotorAzimuth = 45000L;  // maximum rotor azimuth in degrees * 100
const long _maxRotorElevation = 18000L; // maximum rotor elevation in degrees * 100

long _rotorAzimuth = 0L;       // current rotor azimuth in degrees * 100
long _rotorElevation = 0L;     // current rotor azimuth in degrees * 100
long _azimuthTemp = 0L;        // used for gs232 azimuth decoding
long _elevationTemp = 0L;      // used for gs232 elevation decoding  
long _newAzimuth = 0L;         // new azimuth for rotor move
long _newElevation = 0L;       // new elevation for rotor move
long _previousRotorAzimuth = 0L;       // previous rotor azimuth in degrees * 100
long _previousRotorElevation = 0L;     // previous rotor azimuth in degrees * 100

unsigned long _rtcLastDisplayUpdate = 0UL;      // rtc at start of last loop
unsigned long _rtcLastRotorUpdate = 0UL;        // rtc at start of last loop
unsigned long _displayUpdateInterval = 500UL;   // display update interval in mS
unsigned long _rotorMoveUpdateInterval = 100UL; // rotor move check interval in mS

boolean _gs232WActice = false;  // gs232 W command in process
boolean _AZjustmoved = false;  // gs232 W command in process
boolean _ELjustmoved = false;  // gs232 W command in process
int _gs232AzElIndex = 0;        // position in gs232 Az El sequence
long _gs232Azimuth = 0;          // gs232 Azimuth value
long _gs232Elevation = 0;        // gs232 Elevation value
boolean _azimuthMove = false;     // azimuth move needed
boolean _elevationMove = false;   // elevation move needed

String azRotorMovement;   // string for az rotor move display
String elRotorMovement;   // string for el rotor move display


// ------------------------------------------------------------
// ------ Variables for the code to run -----
// ------------------------------------------------------------

char ch;
int valueindex;
String temp = "";
char Command1inBinary[10];
short Command1Length = 9; //Excludes command number char itself
char Command2inBinary[10];
short Command2Length = 9; //Excludes command number char itself
bool Command3;
short Command3Length = 1;
unsigned long RecievedDataArray[3];
unsigned short loopCommand;
int j;

//
// run once at reset
//
void setup()
{
    // initialize rotor control pins as outputs
    pinMode(_G5500UpPin, OUTPUT);
    pinMode(_G5500DownPin, OUTPUT);
    pinMode(_G5500LeftPin, OUTPUT);
    pinMode(_G5500RightPin, OUTPUT);
    // set all the rotor control outputs low
    digitalWrite(_G5500UpPin, LOW);
    digitalWrite(_G5500DownPin, LOW);
    digitalWrite(_G5500LeftPin, LOW);
    digitalWrite(_G5500RightPin, LOW);
    // initialize serial ports:
    Serial.begin(9600);  // for computer printing

    readAzimuth(); // get current azimuth from G-5500
    _previousRotorAzimuth = _rotorAzimuth + 1000;
    readElevation(); // get current elevation from G-5500
    _previousRotorElevation = _rotorElevation + 1000; 
    delay(1000);
//////////////////////////////////////////////////////////////////////////////////////////////////////
    //Serial.print("Initial GS AZ: ");
    //Serial.println(_rotorAzimuth/100);
    //Serial.print("Initial GS EL: ");
    //Serial.println(_rotorElevation/100); 
    //Serial.println("");

    //I2C
    pinMode(13, OUTPUT);
    // initialize i2c as slave
    Wire.begin(SLAVE_ADDRESS);
    // define callbacks for i2c communication
    Wire.onReceive(receiveData);
    //Wire.onRequest(sendData);
}


//
// //*************************************************************************///Start main program loop//*****************************************************************************//
//
void loop() 
{
    //delay(500);
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //Serial.println("");
    //if (Serial.available() > 0){
      String temp=Serial.readString();
      
      Serial.println("");
      Serial.println("Arduino Working");

      //Serial.println(""); 
    //}
    unsigned long rtcCurrent = millis(); // get current rtc value
   
    // check for rtc overflow - skip this cycle if overflow
    if (rtcCurrent > _rtcLastDisplayUpdate){ // overflow if not true    _rotorMoveUpdateInterval
      // update rotor movement if necessary
      //Serial.println("got in2");
    if (rtcCurrent - _rtcLastRotorUpdate > _rotorMoveUpdateInterval){
      _rtcLastRotorUpdate = rtcCurrent; // reset rotor move timer base
      //Serial.println("got in3");
      // AZIMUTH       
      readAzimuth(); // get current azimuth from G-5500

//////////////////////////////////////////////////////////////////////////////////////////////////////
      Serial.print("(deg) Balloon Lat     : ");
      Serial.println(balloonLat,6);
      Serial.print("(deg) Balloon Lon     : ");
      Serial.println(balloonLon,6);
      Serial.print("(cm) Balloon Altit    : ");
      Serial.println(balloonAlt,6);

      Serial.print("(deg) GS Lat          : ");
      Serial.println(groundStationlat,6);
      Serial.print("(deg) GS Lon          : ");
      Serial.println(groundStationlon,6);
      Serial.print("(deg) GS Altitude     : ");
      Serial.println(groundStationAlt,6);
//////////////////////////////////////////////////////////////////////////////////////////////////////
      Serial.print("(deg) AZ Commanded    : ");
      Serial.println(_newAzimuth/100);
      Serial.print("(deg) AZ Read         : ");
      Serial.println(_rotorAzimuth/100);
      Serial.print("(volt)AZ Voltage Read : ");
      Serial.println(((float)analogRead(_azimuthInputPin)*5/1024),6);

      Serial.print("(deg) EL Commanded    : ");
      Serial.println(_newElevation/100);
      Serial.print("(deg) EL Read         : ");
      Serial.println(_rotorElevation/100);
      Serial.print("(volt)EL Voltage Read : ");
      Serial.println(((float)analogRead(_elevationInputPin)*5/1024),6);

      //Serial.print("EL raw Read: ");
      //Serial.println(analogRead(_elevationInputPin));
      //Serial.print("AZ raw Read: ");
      //Serial.println(analogRead(_azimuthInputPin));
  //////////////////////////////////////////////////////////////////////////////////////////////////////
      if (Wire.available()){///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        Serial.println("-----------------------------------------");
        
        //unsigned long FirstNum, el; 
        //unsigned long long lat, lon; 
        
        char CommandChar;
        Command1inBinary[0] = '\0';
        Command2inBinary[0] = '\0';
        Command3 = false;
        while(Wire.available()) {
          i = 0;
          do{
            if(i == 0){
              CommandChar = Wire.read();
              switch(CommandChar){
                case '1':{ // Changes GS GPS
                  loopCommand = Command1Length;
                  Serial.println("case 1a");
                  break;
                } 
                case '2':{ // Changes Balloon GPS
                  loopCommand = Command2Length;
                  Serial.println("case 2a");
                  break;
                } 
                case '3':{ //Asks for Az and El
                  loopCommand = Command3Length;
                  Serial.println("case 3a");
                  break;
                }
              }
            }
            switch(CommandChar){
              case '1':{ // Changes GS GPS
                Command1inBinary[i] = Wire.read();
                Serial.println("case 1b");
                break;
              } 
              case '2':{ // Changes Balloon GPS
                Command2inBinary[i] = Wire.read();
                Serial.print(Command2inBinary[i]);
                Serial.print("----");
                Serial.println(Command2inBinary[i], HEX);
                break;
              } 
              case '3':{ //Asks for Az and El
                Command3 = true;
                Serial.println("case 3b");
                break;
              }
            }
            i++;
          }while(i < loopCommand);
        }
        
        // unsigned long returnedData[3]; //Longintude, Latitude, Altitude
        // char CharsToSend[11];

        // char *writeTo=CharsToSend;
        // unsigned long longBuflatitude;
        // unsigned long longBuflongitude;
        // unsigned int intBufaltitude;

        ////Latitude * 10^5 positive only----------------should be 10^10-----------
        // longBuflatitude = (unsigned long)(29.85782 * 100000);
        // insertBytesFromInt(longBuflatitude, &writeTo, 4);

        ////Longitude * 10^5 positive only max of 109 degrees---should be 10^10-----
        // longBuflongitude = (unsigned long)(45.26487 * 100000);
        // insertBytesFromInt(longBuflongitude, &writeTo, 4);

        ////Altitude * 100--------------------------------------------
        // intBufaltitude = 489 * 100;
        // insertBytesFromInt(intBufaltitude, &writeTo, 3);
        
        // Command2inBinary = String(CharsToSend);
        if(Command1inBinary[0] != '\0'){
          convertBinaryCommands(Command1inBinary, Command1Length, RecievedDataArray);
          groundStationlat = ((float)RecievedDataArray[0])/100000;
          groundStationlon = ((float)RecievedDataArray[1])/100000;
          //groundStationAlt = (((float)RecievedDataArray[2])/1609)/100; //hundreds of meters to hundreds of miles to miles   
          groundStationAlt = (((float)RecievedDataArray[2]))/100; //hundreds of meters to hundreds of miles to miles      
        }
        if(Command2inBinary[0] != '\0'){
          
          convertBinaryCommands(Command2inBinary, Command2Length, RecievedDataArray);
          balloonLat = ((float)RecievedDataArray[0])/100000;
          balloonLon = ((float)RecievedDataArray[1])/100000;
          balloonAlt = (((float)RecievedDataArray[2]))/100; //centimeters to kilometers
          Serial.print("balloonLat: ");
          Serial.println(balloonLat);
          Serial.print("balloonLon: ");
          Serial.println(balloonLon);
          Serial.print("balloonAlt: ");
          Serial.println(balloonAlt);
        }
        if(Command3){
          /////DO STUFF///////////////////////////////////////////////////////////////////////////////////////////////////////////////
          ///////////////DO STUFF/////////////////////////////////////////////////////////////////////////////////////////////////////
          /////////////////////////DO STUFF///////////////////////////////////////////////////////////////////////////////////////////
          ///////////////////////////////////DO STUFF/////////////////////////////////////////////////////////////////////////////////
          /////////////////////////////////////////////DO STUFF///////////////////////////////////////////////////////////////////////
          ////////////////////////////////////////////////////////DO STUFF////////////////////////////////////////////////////////////
        }

       
       
        Serial.print("balloonLat: ");
        Serial.println(balloonLat);
        Serial.print("balloonLon: ");
        Serial.println(balloonLon);
        Serial.print("balloonAlt: ");
        Serial.println(balloonAlt);

        delay(1000);

      }

  //if(balloonLat != 10 && balloonLon != 20 && balloonAlt != 30){
    command = CreateCommand(balloonLon, balloonLat, balloonAlt, groundStationlon, groundStationlat, groundStationAlt);//getCommand(temp.charAt(0));
  //}

  /////////////////////////////////////////
  //command = "W090 090";               ///
  /////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////////
  //Serial.println("Command Sent: " + command);
  for (i=0; i<=command.length(); i++){
    decodeGS232(command.charAt(i));
    //Serial.print(command.charAt(i));
  }
    
         readAzimuth();
         if ( (abs(_rotorAzimuth - _newAzimuth) > _closeEnough) && _azimuthMove ) { // see if azimuth move is required
//////////////////////////////////////////////////////////////////////////////////////////////////////
            Serial.println("-SHOULD MOVE AZ");
            updateAzimuthMove();
            //ERIKS STUFFS
            if (abs(_rotorAzimuth - _newAzimuth) < _closeEnoughSmoothing){
              delay(500); //Slows mount to limit current surges from changing the voltage
              digitalWrite(_G5500LeftPin, LOW);
              digitalWrite(_G5500RightPin, LOW);
            }
            _AZjustmoved = true;
         }
        else{  // no move required - turn off azimuth rotor
//////////////////////////////////////////////////////////////////////////////////////////////////////
           Serial.println("-SHOULD NOT MOVE AZ");
           digitalWrite(_G5500LeftPin, LOW);
           digitalWrite(_G5500RightPin, LOW);
           _azimuthMove = false;
           azRotorMovement = "        ";
           if (_AZjustmoved == true){
              _AZjustmoved = false;
           }
         }
         
         // ELEVATION       
         readElevation(); // get current elevation from G-5500
         // see if elevation move is required
         if ( abs(_rotorElevation - _newElevation) > _closeEnough && _elevationMove ){ // move required{
            Serial.println("-SHOULD MOVE EL");
            updateElevationMove();
            //ERIKS STUFFS
            if (abs(_rotorElevation - _newElevation) < _closeEnoughSmoothing){
              delay(500); //Slows mount to limit current surges from changing the voltage
              digitalWrite(_G5500UpPin, LOW);
              digitalWrite(_G5500DownPin, LOW);
            }
         }
        else{  // no move required - turn off elevation rotor
            Serial.println("-SHOULD NOT MOVE EL");
            digitalWrite(_G5500UpPin, LOW);
            digitalWrite(_G5500DownPin, LOW);
            _elevationMove = false;
            elRotorMovement = "        ";
            if (_ELjustmoved == true){
              _ELjustmoved = false;
           }
         }            
      } // end of update rotor move
   }
}

//*************************************************************************///End main program loop//*****************************************************************************//


//MAIN LOOP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FUNCTIONS


// update elevation rotor move
//
void updateElevationMove()
{          
   // calculate rotor move 
   long rotorMoveEl = _newElevation - _rotorElevation;
   Serial.print("rotorMoveEl:");
   Serial.println(rotorMoveEl);
   if (rotorMoveEl > 0){
      elRotorMovement = "  U ";
      elRotorMovement = elRotorMovement + String(_newElevation / 100);
      digitalWrite(_G5500DownPin, LOW);
      digitalWrite(_G5500UpPin, HIGH);      
   }
   else{           
     if (rotorMoveEl < 0){
       elRotorMovement = "  D ";
       elRotorMovement = elRotorMovement + String(_newElevation / 100);
       digitalWrite(_G5500UpPin, LOW);
       digitalWrite(_G5500DownPin, HIGH);       
     } 
   } 
 }


//
// update azimuth rotor move
//
void updateAzimuthMove()
{          
     // calculate rotor move 
     long rotorMoveAz = _newAzimuth - _rotorAzimuth;
     Serial.print("rotorMoveAz:");
    Serial.println(rotorMoveAz);
     // adjust move if necessary
     if (rotorMoveAz > 18000){ 
        rotorMoveAz = rotorMoveAz - 18000; // adjust move if > 180 degrees
     }
     else{           
       if (rotorMoveAz < -18000){ 
         //rotorMoveAz = rotorMoveAz + 18000; // adjust move if < -180 degrees
         rotorMoveAz = rotorMoveAz + 18000;
       }
     }
     
     if (rotorMoveAz > 0){
        azRotorMovement = "  R ";
        azRotorMovement = azRotorMovement + String(_newAzimuth / 100);
        digitalWrite(_G5500LeftPin, LOW);
        digitalWrite(_G5500RightPin, HIGH);        
     }
     else{           
       if (rotorMoveAz < 0){
         azRotorMovement = "  L ";
         azRotorMovement = azRotorMovement + String(_newAzimuth / 100);
         digitalWrite(_G5500RightPin, LOW); 
         digitalWrite(_G5500LeftPin, HIGH);         
      } 
   }            
}


//
// read azimuth from G5500
//
void readElevation()
{
   //long sensorValue = analogRead(_elevationInputPin);
   long sensorValue = averageRead(_elevationInputPin);
   _rotorElevation = ((sensorValue * 10000) / _elScaleFactor) - _elAdZeroOffset;
}


//
// read azimuth from G5500
//
void readAzimuth()
{
  //long sensorValue = analogRead(_azimuthInputPin);
  long sensorValue = averageRead(_azimuthInputPin);
  _rotorAzimuth = ((sensorValue * 10000) / _azScaleFactor) - _azAdZeroOffset;
}


//
// read azimuth from G5500
//
long averageRead(byte _InputPin)
{
  //long sensorValue = analogRead(_azimuthInputPin);
  int range = 100;//NUMBER OF SAMPLES TO AVERAGE TOGETHER
  long sensorValue = 0;
  for (int i=0; i < range; i++){
    sensorValue = sensorValue + analogRead(_InputPin);
   } 
   sensorValue = (sensorValue/range);
  return sensorValue;
}


//
// decode gs232 commands
//
void decodeGS232(char character)
{
    switch (character){
       case 'w':  // gs232 W command
       case 'W':
       {
          { 
            _gs232WActice = true;
            _gs232AzElIndex = 0;
          }
          break;
       }
       
       // numeric - azimuth and elevation digits
       case '0':  case '1':   case '2':  case '3':  case '4': 
       case '5':  case '6':   case '7':  case '8':  case '9':
       {
          if ( _gs232WActice){
            processAzElNumeric(character);          
          }
       }   
       
       default:{
          // ignore everything else
       }
     }
}


//
// process az el numeric characters from gs232 W command
//
void processAzElNumeric(char character)
{
  
      switch(_gs232AzElIndex){
         case 0:{ // first azimuth character
            _azimuthTemp =(character - 48) * 100;
            _gs232AzElIndex++;
            break;
        } 
        case 1:{
            _azimuthTemp = _azimuthTemp + (character - 48) * 10;
            _gs232AzElIndex++;
            break;
        } 
        case 2:{ // final azimuth character
            _azimuthTemp = _azimuthTemp + (character - 48);
            _gs232AzElIndex++;
            
            // check for valid azimuth 
            if ((_azimuthTemp * 100) > _maxRotorAzimuth){
              _gs232WActice = false;
              _newAzimuth = 0L;
              _newElevation = 0L;
            }           
            break;
        }  
        case 3:{ // first elevation character
            _elevationTemp =(character - 48) * 100;
            _gs232AzElIndex++;
            break;
        } 
        case 4:{
            _elevationTemp = _elevationTemp + (character - 48) * 10;
            _gs232AzElIndex++;
            break;
        } 
        case 5:{ // last elevation character
            _elevationTemp = _elevationTemp + (character - 48);
            _gs232AzElIndex++;
            
            // check for valid elevation 
            if ((_elevationTemp * 100) > _maxRotorElevation){
              _gs232WActice = false;
              _newAzimuth = 0L;
              _newElevation = 0L;
              
            }
            else{ // both azimuth and elevation are ok
              // set up for rotor move
              _newAzimuth = _azimuthTemp * 100;
              _newElevation = _elevationTemp * 100;
              _azimuthMove = true;
              _elevationMove = true;
            }            
            break;
        }             
        
        default:{
           // should never get here
        }         
    } 
}

void receiveData(){
}


// calculates distance along the surface of the Earth between the points at sea level below each of the points
float findDistance(float balloonLon, float balloonLat, float groundStationlon, float groundStationlat) {
  float dlon, dlat, a, c, d;  // ground station = 1  // balloon = 2
  dlon = balloonLon - groundStationlon;
  dlat = balloonLat - groundStationlat;
  a = pow((sin(dlat/2)),2) + cos(groundStationlat) * cos(balloonLat) * pow((sin(dlon/2)),2);
  c = 2 * asin(min(1,sqrt(a)));
  d = Re * c;
  return d;
}
// calculates Azimuth from ground station to balloon
float findAzimuth(float balloonLon, float balloonLat, float groundStationlon, float groundStationlat, float d) {
  float x;  // ground station = 1  // balloon = 2
  x = acos( (sin(balloonLat) - sin(groundStationlat)*cos(d/Re)) / (sin(d/Re)*cos(groundStationlat)) );
  if(sin(balloonLon-groundStationlon) > 0){
    x = (2*pi)-x;
  }
  return x;
}
// calculates Elevation from ground station to balloon
float findElevation(float balloonAlt, float groundStationAlt, float d) {
  float el;  // ground station = 1  // balloon = 2
  el = (atan((balloonAlt - groundStationAlt)/(d))) - (d/(Re));
  //Serial.println("d: ");
  //Serial.println(d);
  return el;
}

String CreateCommand(float balloonLon, float balloonLat, float balloonAlt, float groundStationlon, float groundStationlat, float groundStationAlt){
  String command;
  float d, Azimuth, Elevation;
  
  balloonLat = (balloonLat*pi)/180;              //[deg -> radian]
  balloonLon = (balloonLon*pi)/180;              //[deg -> radian]
  groundStationlat = (groundStationlat*pi)/180;  //[deg -> radian]
  groundStationlon = (groundStationlon*pi)/180;  //[deg -> radian]
  //Serial.println("[meters] groundStationAlt: ");
  //Serial.println(groundStationAlt);
  groundStationAlt = groundStationAlt/1000;   //[m -> km]
  balloonAlt = balloonAlt/100000;   //[cm -> km]
  //Serial.println("[miles] groundStationAlt: ");
  //Serial.println(groundStationAlt);
      
  d = findDistance(balloonLon, balloonLat, groundStationlon, groundStationlat);
  
  Azimuth = findAzimuth(balloonLon, balloonLat, groundStationlon, groundStationlat, d);
  Azimuth = (Azimuth*180)/pi;

  Elevation = findElevation(balloonAlt, groundStationAlt, d);
  Elevation = abs((Elevation*180)/pi);
  //Serial.println("EL: ");
  //Serial.println(Elevation);
  
  command = "W";
  if (Azimuth <= 9){
    command = command + "00" + String(int(Azimuth)) + " ";
  } else if (Azimuth <= 99){
    command = command + "0" + String(int(Azimuth)) + " ";
  } else {
    command = command + String(int(Azimuth)) + " ";
  }
  if (Elevation <= 9){
    command = command + "00" + String(int(Elevation));
  } else if (Elevation <= 99){
    command = command + "0" + String(int(Elevation));
  } else {
    command = command + String(int(Elevation));
  }
  
  return command;
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

void insertBytesFromInt(void* value,unsigned char** byteStart, short numberBytesToCopy){

  unsigned char* valueBytes=value;
  short loopCount=0;
  for(loopCount=0;loopCount<numberBytesToCopy;loopCount++){
  (*byteStart)[loopCount]=valueBytes[loopCount];
  }
  *byteStart+=(short)numberBytesToCopy;
}

void convertBinaryCommands(char *strBinaryCommand, short len, unsigned long returnedData[3]){
  
  char* writeArray=strBinaryCommand;
  char** wrPtr=&writeArray;
 
  returnedData[0] = (unsigned long)getIntFromByte(wrPtr,3);
  returnedData[1] = (unsigned long)getIntFromByte(wrPtr,3);
  returnedData[2] = (unsigned long)getIntFromByte(wrPtr,3);

}
