// This #include statement was automatically added by the Particle IDE.

#include <math.h>

/*------------------------------------------------------------------------
  Particle library to control Adafruit DotStar addressable RGB LEDs.

  Ported by Technobly for Spark Core, Particle Photon, P1, Electron,
  and RedBear Duo.

  ------------------------------------------------------------------------
  -- original header follows ---------------------------------------------
  ------------------------------------------------------------------------

 * Simple strand test for Adafruit Dot Star RGB LED strip.
 * This is a basic diagnostic tool, NOT a graphics demo...helps confirm
 * correct wiring and tests each pixel's ability to display red, green
 * and blue and to forward data down the line.  By limiting the number
 * and color of LEDs, it's reasonably safe to power a couple meters off
 * the VIN pin.  DON'T try that with other code!
 */

/* ======================= includes ================================= */

#include "application.h"

#include "dotstar.h"

#define NUMPIXELS 256 // Number of LEDs in strip

//-------------------------------------------------------------------
// NOTE: If you find that the colors you choose are not correct,
// there is an optional 2nd argument (for HW SPI) and
// 4th arg. (for SW SPI) that you may specify to correct the colors.
//-------------------------------------------------------------------
// e.g. Adafruit_DotStar(NUMPIXELS, DOTSTAR_RGB);
// e.g. Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_RGB);
//
// DOTSTAR_RGB
// DOTSTAR_RBG
// DOTSTAR_GRB
// DOTSTAR_GBR (default)
// DOTSTAR_BRG
// DOTSTAR_BGR

//-------------------------------------------------------------------
// Here's how to control the LEDs from any two pins (Software SPI):
//-------------------------------------------------------------------
#define DATAPIN   D4
#define CLOCKPIN  D5

#define MIC_PIN   A5  // Microphone is attached to this analog pin

#define SAMPLE_WINDOW   20  // Sample window for average level
#define PEAK_HANG 24 //Time of pause before peak dot falls
#define PEAK_FALL 4 //Rate of falling peak dot
#define INPUT_FLOOR 10 //Lower range of analogRead input
#define INPUT_CEILING 500 //Max range of analogRead input, the lower the value the more sensitive (1023 = max)

int peak = NUMPIXELS;      // Peak level of column; used for falling dots
unsigned int sample;

int dotCount = 0;  //Frame counter for peak dot
int dotHangCount = 0; //Frame counter for holding peak dot

Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN);

//-------------------------------------------------------------------
// Here's how to control the LEDs from SPI pins (Hardware SPI):
//-------------------------------------------------------------------
// Hardware SPI is a little faster, but must be wired to specific pins
// (Core/Photon/P1/Electron = pin A5 for data, A3 for clock)
//Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS);

void setup() {
  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP
  Particle.process();
}

// Runs 10 LEDs at a time along strip, cycling through red, green and blue.
// This requires about 200 mA for all the 'on' pixels + 1 mA per 'off' pixel.

//int      head  = 0, tail = -10; // Index of first 'on' and 'off' pixels
//uint32_t color = 0xFF0000;      // 'On' color (starts red)

void loop() {
  Particle.process();

  unsigned long startMillis= millis();  // Start of sample window
  float peakToPeak = 0;   // peak-to-peak level

  unsigned int signalMax = 0;
  unsigned int signalMin = 1023;
  unsigned int c, y;


  // collect data for length of sample window (in mS)
  while (millis() - startMillis < SAMPLE_WINDOW)
  {
    sample = analogRead(MIC_PIN);
    if (sample < 1024)  // toss out spurious readings
    {
      if (sample > signalMax)
      {
        signalMax = sample;  // save just the max levels
      }
      else if (sample < signalMin)
      {
        signalMin = sample;  // save just the min levels
      }
    }
  }
  peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude

  // Serial.println(peakToPeak);


  //Fill the strip with rainbow gradient
  for (int i=0;i<=strip.numPixels()-1;i++){
    strip.setPixelColor(i,Wheel(map(i,0,strip.numPixels()-1,30,150)));
  }


  //Scale the input logarithmically instead of linearly
  c = fscale(INPUT_FLOOR, INPUT_CEILING, strip.numPixels(), 0, peakToPeak, 2);




  if(c < peak) {
    peak = c;        // Keep dot on top
    dotHangCount = 1;    // make the dot hang before falling
  }
  if (c <= strip.numPixels()) { // Fill partial column with off pixels
    drawLine(strip.numPixels(), strip.numPixels()-c, strip.Color(0, 0, 0));
  }

  // Set the peak dot to match the rainbow gradient
  y = strip.numPixels() - peak;

  strip.setPixelColor(y-1,Wheel(map(y,0,strip.numPixels()-1,30,150)));

  strip.show();

  // Frame based peak dot animation
  if(dotHangCount > PEAK_HANG) { //Peak pause length
    if(++dotCount >= PEAK_FALL) { //Fall rate
      peak++;
      dotCount = 2;
    }
  }
  else {
    dotHangCount++;
  }
}

//Used to draw a line between two points of a given color
void drawLine(uint8_t from, uint8_t to, uint32_t c) {
  uint8_t fromTemp;
  if (from > to) {
    fromTemp = from;
    from = to;
    to = fromTemp;
  }
  for(int i=from; i<=to; i++){
    strip.setPixelColor(i, c);
  }
}


float fscale( float originalMin, float originalMax, float newBegin, float
newEnd, float inputValue, float curve){

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;


  // condition curve parameter
  // limit range

  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  /*
   Serial.println(curve * 100, DEC);   // multply by 100 to preserve resolution
   Serial.println();
   */

  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin){
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd;
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine
  if (originalMin > originalMax ) {
    return 0;
  }

  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;

  }
  else     // invert the ranges
  {
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange);
  }

  return rangedValue;
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }

   Particle.process();
}



/*
  strip.setPixelColor(head, color); // 'On' pixel at head
  strip.setPixelColor(tail, 0);     // 'Off' pixel at tail
  strip.show();                     // Refresh strip

  delay(8);                        // Pause 20 milliseconds (~50 FPS)

  if(++head >= NUMPIXELS) {         // Increment head index.  Off end of strip?
    head = 0;                       //  Yes, reset head index to start
    if((color >>= 8) == 0)          //  Next color (R->G->B) ... past blue now?
      color = 0xFF0000;             //   Yes, reset to red
  }
  if(++tail >= NUMPIXELS) tail = 0; // Increment, reset tail index
      Particle.process();
}
*/
