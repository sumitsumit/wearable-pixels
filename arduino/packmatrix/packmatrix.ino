// note this won't compile if it's stored on OneDrive
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
  #include <math.h>
#endif

#define PIN 5
#define BRI 30
#define WIDTH 5
#define HEIGHT 10

// program
static int programnum = 0;
#define NUMPROGRAMS 6
// buttons
static int button1state = 0;

// hearts
static int heartrow = 0;
// rotoflower
static float theta = 0.00;
#define THETAINC 0.05
// wandering star
static float starpos[2] = {0.0, 0.0};
static float starvel[2] = {0.0, 0.0};
static float staracc[2] = {0.0, 0.0};
#define MAXSTARVEL 0.1
#define MAXSTARACC 0.1
#define STARSIZE 1.0
// starry night
static float startime = 0.0;
static int starxlocs[10] = {0,1,2,3,4,0,1,2,3,4};
static int starylocs[10] = {2,6,9,1,4,5,6,0,3,2};
static float starthetaoffset[10] = {0.0,0.6,1.2,1.8,2.4,3.0,3.6,4.2,4.8,5.4};
#define STARTIMEINC 0.03
// colorwave
static float wavetheta = 0.0;
#define WAVETHETASTEP 0.5
#define WAVETHETAINC 0.05
static float waverval = 0.5;
static float wavegval = 0.5;
static float wavebval = 0.5;
static float waverinc = 0.0012;
static float waveginc = 0.0023;
static float wavebinc = 0.0034;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
// Adafruit_NeoPixel strip = Adafruit_NeoPixel(50, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(50, PIN, NEO_RGB + NEO_KHZ400);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code

  Serial.begin(9600);
  pinMode(A0, INPUT);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}


void loop() {
  checkbuttons();
 if (programnum == 0) {
    wanderingstar(10);
  }
  if (programnum == 1) {
    scrollHeart(strip.Color(BRI,0,0),100);
  }
  if (programnum == 2) {
    rotoflower(10);
  }
  if (programnum == 3) {
    starrynight(10);
  }
  if (programnum == 4) {
    colorwave(10);
  }
  if (programnum == 5) {
    alloff(10);
  }
}

void checkbuttons()
{
  // check button 2 - change animation
  int button1val = analogRead(A0);
  //Serial.println(button1val);
  if (button1val < 512 && button1state == 0)
  {
    // button 1 pressed brings the voltage low
    button1state = 1;

    // advance sequence
    programnum = (programnum + 1)%NUMPROGRAMS;
  }
  if (button1val > 512 && button1state == 1)
  {
    // button 1 released
    button1state = 0;
  }

}

void alloff(uint8_t wait)
{
  int numstars = sizeof(starxlocs)/sizeof(int);
  for (uint16_t i=0; i < strip.numPixels(); i=i+1) {
    strip.setPixelColor(i,strip.Color(0,0,0));
  }

  strip.show();
  delay(wait);
}

void colorwave(uint8_t wait)
{
  waverval = waverval + waverinc;
  wavegval = wavegval + waveginc;
  wavebval = wavebval + wavebinc;
  if (waverval > 0.95 or waverval < 0.05) {
    waverinc = -waverinc;
  }
  if (wavegval > 0.95 or wavegval < 0.05) {
    waveginc = -waveginc;
  }
  if (wavebval > 0.95 or wavebval < 0.05) {
    wavebinc = -wavebinc;
  }
  if (waverval < 0) {
    waverval = 0;
  }
  if (wavebval < 0) {
    wavebval = 0;
  }
  if (wavegval < 0) {
    wavegval = 0;
  }
  float denom = waverval + wavegval + wavebval;
  float normwaverval = waverval/denom;
  float normwavegval = wavegval/denom;
  float normwavebval = wavebval/denom;
  for (uint16_t i=0; i < strip.numPixels(); i=i+1) {
    float x = xcoord_from_pixelnum(i);
    float y = ycoord_from_pixelnum(i);
    float ival = (sin(wavetheta + y*WAVETHETASTEP)+1)/2.0;
    strip.setPixelColor(i,strip.Color(ival*normwaverval*BRI,ival*normwavegval*BRI,ival*normwavebval*BRI));
  }
  wavetheta = wavetheta - WAVETHETAINC;
  
  // update array
  strip.show();
  delay(wait);
}

void starrynight(uint8_t wait)
{
  int numstars = sizeof(starxlocs)/sizeof(int);
  for (uint16_t i=0; i < strip.numPixels(); i=i+1) {
    strip.setPixelColor(i,strip.Color(0,0,0));
  }
   for (int starnum = 0; starnum < numstars; starnum++) {
    int pixelnum = pixelnum_from_rowcol(starylocs[starnum],starxlocs[starnum]);
    float thetaoffset = starthetaoffset[starnum];
    float ival = BRI*( (sin(startime+thetaoffset)+1)/2.0 );
    // update location if ival < 0
    if (ival < 0.1) {
      starxlocs[starnum] = random(WIDTH-1);
      starylocs[starnum] = random(HEIGHT-1);
    }
    if (ival < 1.0) {
      ival = 0.0;
    }
    uint32_t pcolor = strip.Color(round(ival),round(ival),round(ival));
    strip.setPixelColor(pixelnum,pcolor);
 }
 startime += STARTIMEINC;
  
  // update array
  strip.show();
  delay(wait);
}

void wanderingstar(uint8_t wait)
{
  // update acceleration, velocity, position
  staracc[0] = staracc[0] + 0.01*((float)(random(100))/100.0 - 0.5);
  staracc[1] = staracc[1] + 0.01*((float)(random(100))/100.0 - 0.5);
  starvel[0] = starvel[0] + staracc[0];
  starvel[1] = starvel[1] + staracc[1];
  // bounds checking
  if (staracc[0] > MAXSTARACC) {
    staracc[0] = MAXSTARACC;
  }
  if (staracc[0] < -MAXSTARACC) {
    staracc[0] = -MAXSTARACC;
  }
  if (staracc[1] > MAXSTARACC) {
    staracc[1] = MAXSTARACC;
  }
  if (staracc[1] < -MAXSTARACC) {
    staracc[1] = -MAXSTARACC;
  }
  if (starvel[0] > MAXSTARVEL) {
    starvel[0] = MAXSTARVEL;
  }
  if (starvel[0] < -MAXSTARVEL) {
    starvel[0] = -MAXSTARVEL;
  }
  if (starvel[1] > MAXSTARVEL) {
    starvel[1] = MAXSTARVEL;
  }
  if (starvel[1] < -MAXSTARVEL) {
    starvel[1] = -MAXSTARVEL;
  }
  // reflection  
  if (starpos[0] < -1.5 or starpos[0] > 1.5) {
    starvel[0] = -starvel[0];
    staracc[0] = -staracc[0];
  }
  if (starpos[1] < -4 or starpos[1] > 4) {
    starvel[1] = -starvel[1];
    staracc[1] = -staracc[1];
  }
  starpos[0] = starpos[0] + starvel[0];
  starpos[1] = starpos[1] + starvel[1];

  //Serial.print(starpos[0]);
  //Serial.print(",");
  //Serial.print(starpos[1]);
  //Serial.println();

  // update pixels
  for (uint16_t i=0; i < strip.numPixels(); i=i+1) {
    float x = xcoord_from_pixelnum(i);
    float y = ycoord_from_pixelnum(i);
    float rsquared =  (x-starpos[0])*(x-starpos[0]) + (y-starpos[1])*(y-starpos[1]);
    float ival = BRI*(exp(-rsquared/STARSIZE));
    if (ival < 1.0) {
      ival = 0.0;
    }
    uint32_t pcolor = strip.Color(round(ival),round(ival),round(ival));
    strip.setPixelColor(i,pcolor);
  }

  // update array
  strip.show();
  delay(wait);
}

void rotoflower(uint8_t wait) 
{
  for (uint16_t i=0; i < strip.numPixels(); i=i+1) {
    float x = xcoord_from_pixelnum(i);
    float y = ycoord_from_pixelnum(i);
    float ptheta = atan2(y,x);
    float rval = 0.5*BRI*((1+sin(ptheta+theta))/2.0);
    float bval = 0.5*BRI*((1+cos(ptheta+theta))/2.0);
    uint32_t pcolor = strip.Color(round(rval),0,round(bval));
    strip.setPixelColor(i,pcolor);
  }
  theta = theta + THETAINC;
  strip.show();
  delay(wait);
}

void gradientwipe(uint8_t wait) {
  for (uint16_t i=0; i < strip.numPixels(); i=i+1) {
    Serial.print(i);
    Serial.print(" ");
    float x = xcoord_from_pixelnum(i);
    Serial.print(x);
    Serial.print(",");
    float y = ycoord_from_pixelnum(i);
    Serial.print(y);
    Serial.print(" ");
    float rval = 5*(4.5+y);
    Serial.print((int)rval);
    Serial.print(",");
    float bval = 5*(2+x);
    Serial.print((int)bval);
    Serial.print("\n");
    uint32_t pcolor = strip.Color((int)rval,0,(int)bval);
    strip.setPixelColor(i,pcolor);
  }
  strip.show();
  delay(wait);
}

float xcoord_from_pixelnum(uint16_t pixelnum)
{
  uint16_t colnum = pixelnum%WIDTH;
  uint16_t rownum = pixelnum/WIDTH;
  if ( (rownum % 2) == 1) {
    colnum = WIDTH-1-colnum;
  }
  return ( (float)colnum - float(WIDTH-1)/2.0 );
}

float ycoord_from_pixelnum(uint16_t pixelnum)
{
  uint16_t rownum = pixelnum/WIDTH;
  return(  (float)rownum - ((float)(HEIGHT-1))/2.0 );
}

int pixelnum_from_rowcol(int row, int col)
{
  int coloffset = col;
  if ( (row % 2) == 1 ) {
    coloffset = WIDTH-1-coloffset;
  }
  return row*WIDTH+coloffset;
}

void scrollHeart(uint32_t color, uint8_t wait)
{
  drawHeart(HEIGHT-1-heartrow, color);
  delay(wait);
  drawHeart(HEIGHT-1-heartrow, strip.Color(0,0,0));
  heartrow = (heartrow+1)%HEIGHT;
}

void drawHeart(uint16_t rownum, uint32_t color)
{
  int16_t drawrow = rownum;
  //Serial.print(drawrow);
  // row 0
  strip.setPixelColor(drawrow*WIDTH+1,color);
  strip.setPixelColor(drawrow*WIDTH+3,color);
  // row 1
  drawrow = (drawrow - 1);
  if (drawrow < 0) {
    drawrow = HEIGHT+drawrow;
  }
  //Serial.print(drawrow);
  strip.setPixelColor(drawrow*WIDTH+0,color);
  strip.setPixelColor(drawrow*WIDTH+1,color);
  strip.setPixelColor(drawrow*WIDTH+2,color);
  strip.setPixelColor(drawrow*WIDTH+3,color);
  strip.setPixelColor(drawrow*WIDTH+4,color);
  // row 2
  drawrow = (drawrow - 1);
  if (drawrow < 0) {
    drawrow = HEIGHT+drawrow;
  }
  //Serial.print(drawrow);
  strip.setPixelColor(drawrow*WIDTH+1,color);
  strip.setPixelColor(drawrow*WIDTH+2,color);
  strip.setPixelColor(drawrow*WIDTH+3,color);
  // row 3
  drawrow = (drawrow - 1);
  if (drawrow < 0) {
    drawrow = HEIGHT+drawrow;
  }
  //Serial.print(drawrow);
  strip.setPixelColor(drawrow*WIDTH+2,color);
  strip.show();
  //Serial.print("\n");
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
