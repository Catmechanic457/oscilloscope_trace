#include "image.h"

/*
Compatible with ATmega328p based boards

The signal is generated at PB1.
For most boards, PB1 is digital pin 9
*/

// this pin will be high only on the first frame
// this is to aid oscilloscope triggering
#define TRIGGER_PIN 8

void setup() {
  // set trigger pin as output
  pinMode(TRIGGER_PIN, OUTPUT);

  // set PB1 as output
  DDRB |= 0b00000010;

  /* configure timer1:
    output compare A:          non-inverting PWM
    output compare B:          disconnected
    waveform generation mode:  fast-pwm
    clock-prescaler:           x1
  */
  TCCR1A = 0b10000001;
  TCCR1B = 0b00001001;
}

template <typename T>
void draw_line(T line, uint8_t size) {

  static constexpr uint8_t max_blocks = 64;

  T blocks[2][max_blocks];

  uint8_t block_count = 0;
  uint8_t i = 0;
  while (i < size) {
    // find start of block
    if ((line >> i) % 2 == 1) {
      blocks[0][block_count] = i;
      i++;
      // find end of block
      while ((line >> i) % 2 == 1 && i < size)
        i++;
      blocks[1][block_count] = i;
      block_count++;
      if (block_count >= max_blocks) break;
    }
    else i++;
  }

  static constexpr uint32_t display_period = 262144; // how many microseconds a pixel spans
  static constexpr uint32_t resolution = 8; // how many blocks to display in a pixel

  static constexpr uint32_t block_period = display_period / resolution;

  if (block_count == 0) {
    delay(display_period / 1000);
    return;
  }

  for (uint32_t i = 0; i < resolution; i++) {

    // get the top and bottom value of this pixel block
    T top, bottom;
    top = blocks[1][i % block_count];
    bottom = blocks[0][i % block_count];

    // map between 0 and 255
    uint16_t top_val = (256 / size) * top;
    uint16_t bottom_val = (256 / size) * bottom;
    if (top_val == 256) top_val = 255;

    uint8_t values[2] = {(uint8_t)top_val, (uint8_t)bottom_val};
    uint32_t n = 0;

    // loop through the pixel blocks until one "block period" has passed
    // for each pair of loops, modulate the PWM between the top and bottom values
    uint32_t t = micros();
    while (micros() - t < block_period) {
      OCR1A = 255 - values[n % 2];
      delayMicroseconds(4000);
      n++;
    }
  }
}

// current frame
unsigned n = 0;

// image source
auto& image = image_heart;
uint32_t image_width = 8;
uint8_t image_height = 8;

void loop() {

  // on the first frame, set the trigger pin high
  if ((n % image_width) == 0) {
    n = 0;
    digitalWrite(TRIGGER_PIN, HIGH);
  }
  else digitalWrite(TRIGGER_PIN, LOW);

  // draw a single vertical slice of the image
  draw_line(image_heart[n], image_height);
  
  // next frame
  n++;
}
