
#include <math.h>

const float r1 = 2.7;   // Value of R1 in the Flexinol voltage divider
const byte msbPin = 9;  // The highest numbered pin of the 8 bit output to the DAC

float vIn;              // The measured voltage going into the Flexinol voltage divider
float vOut;             // The measured voltage between R1 and the Flexinol
float rFlexinol;        // The calculated resistance of the Flexinol

byte dac;               // The byte value to write to the DAC 
byte thisPin;           // Used in the routine to set the individual bits of the DAC
byte thisBit;           // Used in the routine to set the individual bits of the DAC

float avgActual;        // Used in testing to track the average resistance readings
float delta;            // Used in testing to track the difference between the target and actual values

int index;                    // Used to reference array variables
float resistanceValues[11];   // Target resistance values
int dacValuesWarming[11];     // Calibrated DAC values when Flexinol is warming  
int dacValuesCooling[11];     // Calibrated DAC values when Flexinol is cooling


/**************************************************************************************************
  Functional subroutines:

  bytewrite:  takes an 8-bit value and sends each bit to an individual Arduino pin
              to drive the DAC

  calcResist: takes ADC readings at A0 and A1 and uses the results to calculate the
              current resistance of the Flexinol
**************************************************************************************************/

void setup() {
  Serial.begin(9600);
  pinMode (2,  OUTPUT);
  pinMode (3,  OUTPUT);
  pinMode (4,  OUTPUT);
  pinMode (5,  OUTPUT);
  pinMode (6,  OUTPUT);
  pinMode (7,  OUTPUT);
  pinMode (8,  OUTPUT);
  pinMode (9,  OUTPUT);
}

void loop(){
  calibrate();
  test();
}

/**************************************************************************************************
  Calibration:

  This subroutine first measures the highest and lowest resistance readings across the range of
  potential voltages. It then calculates a set of resistance values at regular intervals.  Finally
  DAC values are cycled through continuously testing for the best value to achieve each of the
  calculated resistances.
**************************************************************************************************/

void calibrate(){
  Serial.println ("Working");
  Serial.println (" ");
  resistanceValues[0] = 7;   // Seed the high and low resistance values with values well outside
  resistanceValues[10] = 13; // the expected range
  dac = 0;
  byteWrite();
  delay(10000); 

/**************************************************************************************************
  Find the high and low resistance values and a corresponding DAC value
**************************************************************************************************/

  for (dac = 100 ; dac <= 210; dac++)   // Range determined experimentally
  {
    byteWrite();
    delay (500);
    calcResist();
    if (rFlexinol > resistanceValues[0])
    {
      resistanceValues[0] = rFlexinol;
      dacValuesWarming[0] = dac;
    } 
    if (rFlexinol < resistanceValues[10])
    {
      resistanceValues[10] = rFlexinol;
      dacValuesWarming[10] = dac;
    } 
  }

/**************************************************************************************************
  Intermediate resistance values (Flexinol positions) are calculated based on the high and low
  The results are stored in an array and output to the serial terminal
**************************************************************************************************/

  for (index = 0; index <= 10; index++)
  {
    resistanceValues[index] = ((resistanceValues[10] - resistanceValues[0]) * index * .1) + resistanceValues[0];
    Serial.print (index * 10, DEC);
    Serial.print ("% Contracted Resistance = ");
    Serial.println (resistanceValues[index], DEC);
  }

  Serial.println (" ");

/**************************************************************************************************
  Remove power and allow the Flexinol to cool
**************************************************************************************************/

  dac = 0;
  byteWrite();
  delay (3000);

/**************************************************************************************************
  Set values 1-9 in the warming values table to maximum to test against
**************************************************************************************************/

  for (index = 1; index <= 9; index++)
  {
    dacValuesWarming[index] = 255;
  }


/**************************************************************************************************
  Slowly increase voltage to the circuit - the first time a target resistance value is recorded
  place the corresponding DAC value in the warming index
**************************************************************************************************/

  dac = dacValuesWarming[0];
  do
  {
    byteWrite();
    delay(500);
    calcResist();

    for (index = 1; index <= 9; index++)
    {  
      if (rFlexinol <= resistanceValues[index] && dacValuesWarming[index] == 255)
      {
        dacValuesWarming[index] = dac - 1;
      }
    }
    ++dac;  
  }
  while (dacValuesWarming[9] == 255);   // repeat until all values mapped

/**************************************************************************************************
  Send the results of the warming calibration to the serial terminal
**************************************************************************************************/

  for (index = 0; index <= 10; index++)
  {
    Serial.print (index * 10, DEC);
    Serial.print ("% Contracted: DAC Value Warming = ");
    Serial.println (dacValuesWarming[index], DEC);
  }

  Serial.println (" "); 

/**************************************************************************************************
 The cooling calibration simply reverses the above warming calibration, steadily reducing the
 voltage and recording the DAC value the first time a target value is reached
 Note one difference: the cooling index checks to be sure that the cooling value is
 in fact less than the warming value, and if not keeps cycling 
**************************************************************************************************/


  for (index = 10; index >= 0; index--)
  {
    dacValuesCooling[index] = 0;
  }


  do
  {
    byteWrite();
    delay(500);
    calcResist();

    for (index = 0; index <= 10; index++)
    {  
      if (rFlexinol >= resistanceValues[index] && dacValuesCooling[index] == 0 && dac + 1 < dacValuesWarming[index])
      {
        dacValuesCooling[index] = dac + 1;
      }
    }
    --dac;  
  }
  while (dacValuesCooling[0] == 0 && dac > 100);

  for (index = 0; index <= 10; index++)
  {
    Serial.print (index * 10, DEC);
    Serial.print ("% Contracted: DAC Value Cooling = ");
    Serial.println (dacValuesCooling[index], DEC);
  }

}


/**************************************************************************************************
 Test

 The test routine is more straightforward than it may appear at first.  The circuit cycles through
 the target resistance values and attempts to hold the position.  Resistance is calculated.  If the
 Flexinol is too relaxed, the warming value is output to the DAC.  If the Flexinol is too
 contracted then the cooling value is used.

 The avgActual and delta variables and associated calculations are used to track average values
 and deviations from the target which are output to the terminal along with the other data.
**************************************************************************************************/

void test(){
  for (index = 1; index <= 9; index ++)
  {
    Serial.println (" ");
    Serial.print ("Testing "); 
    Serial.print (index * 10);
    Serial.println("%");


    dac = dacValuesWarming[index];
    for (int outerLoop = 0; outerLoop <= 4; outerLoop ++)
    {
      delta = 0;
      avgActual = 0;
      for (int innerLoop = 0; innerLoop <= 49; innerLoop ++)
      {
        byteWrite();
        delay (200);
        calcResist();
        avgActual = avgActual + rFlexinol;
        
        if (rFlexinol < resistanceValues[index])
        {
          dac = dacValuesCooling[index];   
        } 
        else {
          dac = dacValuesWarming[index];   
        }
        delta = delta + abs(resistanceValues[index] - rFlexinol);
      } 

      delta = delta /50;
      avgActual = avgActual /50;

      Serial.print ("Target: ");   
      Serial.print (resistanceValues[index], DEC);    
      Serial.print ("   Average Actual: ");
      Serial.print (avgActual, DEC);
      Serial.print ("   Delta: ");
      Serial.print (avgActual - resistanceValues[index], DEC);
      Serial.print ("   Average Deviation: ");
      Serial.println (delta, DEC);
    }  
  }
}


/**************************************************************************************************
  byteWrite takes the value in dac and outputs it to the 8-bit resistor ladder wired
  to Arduino pins 2-9.
**************************************************************************************************/

void byteWrite(){
  for (byte j = 0; j <= 7; j++){
    thisPin = msbPin - j;
    thisBit = bitRead(dac, 7 - j);
    digitalWrite (thisPin, thisBit);
  }  
}


/**************************************************************************************************
  calcResist takes ADC readings at A0 and A1, converts them to voltages and uses the results to
  calculate the resistance of the Flexinol - the constant value of 0.004883 used here is equal to
  the reference voltage of 5 divided by the ADC resolution of 1024
**************************************************************************************************/

void calcResist(){
  vIn = analogRead (A0);  // take a throw-away reading to reset the ADC after switching pins
  delay (10);
  vIn = analogRead (A0);
  vIn = vIn * 0.004883;
  
  vOut = analogRead (A1);
  delay (10);
  vOut = analogRead (A1);
  vOut = vOut * 0.004883;
  
  rFlexinol = (r1 * vOut) / (vIn - vOut);
}
