#include <math.h>
//Schematic:
// [Ground] ---- [10k-Resistor] -------|------- [Thermistor] ---- [+5v]
//                                                             |
//                                                     Analog Pin 0

double Thermistor(int RawADC) {
 // Inputs ADC Value from Thermistor and outputs Temperature in Celsius
 //  requires: include <math.h>
 // Utilizes the Steinhart-Hart Thermistor Equation:
 //    Temperature in Kelvin = 1 / {A + B[ln(R)] + C[ln(R)]^2 + D[ln(R)]^3}
 //    where A = 0.001026853381291383, B = 0.0002398699085819556 and C = -0.00000007884414548067204, D = 0.0000001594372457358320  (calculated using R-T table and Coeff program
 
 long Resistance;  double Temp;  // Dual-Purpose variable to save space.
 Resistance=10000.0*((1023.0/RawADC) - 1);  // Assuming a 10k Thermistor.  Calculation is actually: Resistance = (1023 /ADC -1) * BalanceResistor
// For a GND-Thermistor-PullUp--Varef circuit it would be Rtherm=Rpullup/(1023.0/ADC-1)

 Temp = log(Resistance); // Saving the Log(resistance) so not to calculate it 4 times later. // "Temp" means "Temporary" on this line.
 Temp = 1 / (0.001026853381291383 + (0.0002398699085819556 * Temp) + (-0.00000007884414548067204 * Temp * Temp) + (0.0000001594372457358320*Temp*Temp*Temp));   // Now it means both "Temporary" and "Temperature"
 Temp = Temp - 273.15;  // Convert Kelvin to Celsius                                         // Now it only means "Temperature"

 // BEGIN- Remove these lines for the function not to display anything
  //Serial.print("ADC: "); Serial.print(RawADC); Serial.print("/1023");  // Print out RAW ADC Number
  //Serial.print(", Volts: "); printDouble(((RawADC*5)/1023.0),3);   
  //Serial.print(", Resistance: "); Serial.print(Resistance); Serial.print("ohms");
 // END- Remove these lines for the function not to display anything

 // Uncomment this line for the function to return Fahrenheit instead.
 //Temp = (Temp * 9.0)/ 5.0 + 32.0; // Convert to Fahrenheit
 return Temp;  // Return the Temperature
}

void printDouble(double val, byte precision) {
  // prints val with number of decimal places determine by precision
  // precision is a number from 0 to 6 indicating the desired decimal places
  // example: printDouble(3.1415, 2); // prints 3.14 (two decimal places)
  Serial.print (int(val));  //prints the int part
  if( precision > 0) {
    Serial.print("."); // print the decimal point
    unsigned long frac, mult = 1;
    byte padding = precision -1;
    while(precision--) mult *=10;
    if(val >= 0) frac = (val - int(val)) * mult; else frac = (int(val) - val) * mult;
    unsigned long frac1 = frac;
    while(frac1 /= 10) padding--;
    while(padding--) Serial.print("0");
    Serial.print(frac,DEC) ;
  }
}

void setup() {
 Serial.begin(115200);
}

#define ThermistorPIN 0   // Analog Pin 0
double temp;
void loop() {
 temp=Thermistor(analogRead(ThermistorPIN));           // read ADC and convert it to Celsius
 Serial.print(", Celsius: "); printDouble(temp,3);     // display Celsius
 temp = (temp * 9.0)/ 5.0 + 32.0;                      // converts to Fahrenheit
 Serial.print(", Fahrenheit: "); printDouble(temp,3);  // display Fahrenheit
 Serial.println("");                                   // End of Line
 delay(100);                                           // Delay to not Serial.print faster than the serial connection can output
}

