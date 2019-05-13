// storefront.ino - main arduino sketch for storefront lighting project
// note this won't compile if it's stored on OneDrive
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
  #include <math.h>
#endif


// installation-specific constants - adjust these when using a different strip length
#define OUTPUT_PIN 5
#define RELAY_PIN 3
#define BANK1_START 0
#define BANK1_END 28
#define BANK2_START 29
#define BANK2_END 49
#define BUTTON_AVAILABLE true

// Multiglow constants
#define BRI 30
#define NUMBLOBS 4
#define COLOR_INCR_R 0.137
#define COLOR_INCR_B 0.155
#define COLOR_INCR_G 0.174
#define MULTIGLOW_THETA_INCR 0.003
#define BLOB_THETA_INCR 0.01
#define BLOB_MAX_VEL 0.005
#define BLOB_SPATIAL_DECAY 400
#define BLOB_SUBSAMPLE 2
#define FIRE_GREEN_LIMIT 0.75

// program
static int programnum = 0;
#define NUMPROGRAMS 4
// buttons
static int button1state = 0;
// multiglow
static float bloblocs[NUMBLOBS];
static float blobvels[NUMBLOBS];
static float blobthetas[NUMBLOBS];
static float blobcolors_r[NUMBLOBS];
static float blobcolors_g[NUMBLOBS];
static float blobcolors_b[NUMBLOBS];
static float multiglow_theta = 0.0;
// relay
static int relay_state = 0;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
// Adafruit_NeoPixel strip = Adafruit_NeoPixel(50, OUTPUT_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(50, OUTPUT_PIN, NEO_RGB + NEO_KHZ400);

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

  // set initial thetas of blobs 
  for (int i=0; i<NUMBLOBS; i++) {
    blobthetas[i] = float(random(628))/100.0;
    bloblocs[i] = random(100)/100.0;
    blobvels[i] = -0.01 + float(random(200))/10000.0;
    blobcolors_r[i] = float(random(100))/100.0;
    blobcolors_g[i] = float(random(100))/100.0;
    blobcolors_b[i] = float(random(100))/100.0;
  }

  Serial.begin(9600);
  pinMode(A0, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}


void loop() {
  if (BUTTON_AVAILABLE) {
    checkbuttons();
  }
  if (programnum == 0) {
    multiglow(10);
  }
  if (programnum == 1) {
    checkbanks(10);
  }
  if (programnum == 2) {
    allon(10);
  }
  if (programnum == 3) {
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

void lamp_on()
{
  if (relay_state == 0) {
    digitalWrite(RELAY_PIN,HIGH);
    relay_state = 1;
  }
}

void lamp_off()
{
  if (relay_state == 1) {
    digitalWrite(RELAY_PIN,LOW);
    relay_state = 0;
  }

}


void multiglow(uint8_t wait)
{
  // update global theta
  multiglow_theta += MULTIGLOW_THETA_INCR;

  // update blobs
  for (uint16_t blobnum = 0; blobnum < NUMBLOBS; blobnum++) {
    // update blob thetas
    blobthetas[blobnum] += BLOB_THETA_INCR;
    // update blob velocities
    blobvels[blobnum] += -0.001 + float(random(200))/100000.0;
    if (abs(blobvels[blobnum]) > BLOB_MAX_VEL) {
      blobvels[blobnum]*=0.9;
    }
    // update blob positions
    bloblocs[blobnum] += blobvels[blobnum];
    if (bloblocs[blobnum] >= 1.0) {
      bloblocs[blobnum] = 1.0;
      // reflect position if it hits an end
      if (blobvels[blobnum]>0) {
        blobvels[blobnum] = -blobvels[blobnum];
      }
    }
    if (bloblocs[blobnum] <= 0.0) {
      bloblocs[blobnum] = 0.0;
      // reflect velocity if it hits an end
      if (blobvels[blobnum]<0) {
        blobvels[blobnum] = -blobvels[blobnum];
      }
    }
    // update blob color
    blobcolors_r[blobnum] += COLOR_INCR_R;
    if (blobcolors_r[blobnum] > 1.0) {
      blobcolors_r[blobnum] -= 1.0;
    }
    blobcolors_g[blobnum] += COLOR_INCR_G;
    if (blobcolors_g[blobnum] > 1.0) {
      blobcolors_g[blobnum] -= 1.0;
    }
    blobcolors_g[blobnum] += COLOR_INCR_R;
    if (blobcolors_g[blobnum] > 1.0) {
      blobcolors_g[blobnum] -= 1.0;
    }
  }

  // update pixels in bank 1
  float rval = 0;
  float gval = 0;
  float bval = 0;
  if (cos(multiglow_theta) > -.5) {
    lamp_off();
    for (uint16_t i= BANK1_START; i <= BANK1_END; i=i+1) {
      if (i % BLOB_SUBSAMPLE == 0) {
        // reset values for accumulation
        rval = 0;
        gval = 0;
        bval = 0;
        for (uint16_t blobnum=0; blobnum<NUMBLOBS; blobnum++) {
          // render blob blobnum for position i
          float cursorpos = float(i-BANK1_START)/float(BANK1_END-BANK1_START+1);
          float rsquared = (bloblocs[blobnum]-cursorpos)*(bloblocs[blobnum]-cursorpos);
          float blob_brightness_dynamic = (1+cos(blobthetas[blobnum]))*(.5+cos(multiglow_theta));
          float brightness = exp(-rsquared*BLOB_SPATIAL_DECAY)*blob_brightness_dynamic;
          // strip.setPixelColor(i,strip.Color(int(brightness*blobcolors_r[blobnum]), 
          //                                   int(brightness*blobcolors_g[blobnum]), 
          //                                   int(brightness*blobcolors_b[blobnum])));

          rval += brightness*BRI;
          gval += brightness*BRI;
          bval += brightness*BRI;
        }
      }
      strip.setPixelColor(i,strip.Color(int(rval),int(gval),int(bval)));      
    }
  } else {
    for (uint16_t i= BANK1_START; i <= BANK1_END; i=i+1) {
        strip.setPixelColor(i,strip.Color(0,0,0));
    }
  }
  // update pixels in bank 2
  float fire_brightness = -3*cos(multiglow_theta);
  if (fire_brightness > 0.0) {
    // turn lamp on/off at peak of cycle
    if (fire_brightness > 2.0) {
      lamp_on();
    } else {
      lamp_off();
    }
    // set pixels to fire colors modulated by fire brightness
    for (uint16_t i= BANK2_START; i <= BANK2_END; i=i+1) {
      strip.setPixelColor(i,strip.Color(int(BRI*fire_brightness),int(random(BRI)*fire_brightness*FIRE_GREEN_LIMIT),0));
    }
  } else {
    // otherwise turn all pixels off
    for (uint16_t i= BANK2_START; i <= BANK2_END; i=i+1) {
      strip.setPixelColor(i,strip.Color(0,0,0));
    }
  }

  strip.show();
  delay(wait);
}

void checkbanks(uint8_t wait)
{
  for (uint16_t i= BANK1_START; i <= BANK1_END; i=i+1) {
    strip.setPixelColor(i,strip.Color(2*BRI,BRI,BRI));
  }
  for (uint16_t i= BANK2_START; i <= BANK2_END; i=i+1) {
    strip.setPixelColor(i,strip.Color(BRI,2*BRI,BRI));
  }

  strip.show();
  delay(wait);
}

void allon(uint8_t wait)
{
  for (uint16_t i=0; i < strip.numPixels(); i=i+1) {
    strip.setPixelColor(i,strip.Color(2*BRI,BRI,BRI));
  }

  strip.show();
  delay(wait);
}

void alloff(uint8_t wait)
{
  for (uint16_t i=0; i < strip.numPixels(); i=i+1) {
    strip.setPixelColor(i,strip.Color(0,0,0));
  }

  strip.show();
  delay(wait);
}

