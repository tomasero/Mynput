#include "mbed.h"
#include "MMA8451Q.h"
#include "Timer.h"
#include "USBKeyboard.h"
#include "PinDetect.h"
#include "PololuLedStrip.h"

// define I2C Pins and address for KL25Z. Taken from default sample code.
PinName const SDA = PTE25;
PinName const SCL = PTE24;
Timer timer;
int timer_begin;

#define WRIST_LED_COUNT 16
#define CHEST_LED_COUNT 24
#define MMA8451_I2C_ADDRESS (0x1d<<1)
#define COUNTER_MOD 100
#define X_MOVEMENT_REPEATS 5

#define LEAN_LEFT_THRESH -0.4
#define LEAN_RIGHT_THRESH 0.4
#define LEAN_BACK_THRESH 0.5
#define JUMP_THRESH 0.4
#define LASER_THRESH 0.4
#define SHIELD_THRESH 0.41
const char LEFT =  'A';
const char RIGHT = 'D';
const char LASER = 'J';
const char SHIELD = 'K';
const char SLOW = 'L';
const char ENTER = '\n';
const char SPACE = ' ';

//serial connection to PC via USB
Serial pc(USBTX, USBRX);

PololuLedStrip wristLedStrip(D6);
PololuLedStrip chestLedStrip(D7);

//rgb_color colors[WRIST_LED_COUNT];

USBKeyboard keyboard;
AnalogIn rightArm(A0);
AnalogIn wrist(A2);
PinDetect on_off(D15);
bool on = false;
int counter = 0;


void on_off_pressed() {
    on = !on;
}

rgb_color getColor( char color, int intensity ) {
  rgb_color pixelColor;
  switch (color) {
    case 'r':
      pixelColor = (rgb_color){ intensity, 0, 0 };
      break;
    case 'g':
      pixelColor = (rgb_color){ 0, intensity, 0 };
      break;
    case 'b':
      pixelColor = (rgb_color){ 0, 0, intensity };
      break;
    case 'w':
      pixelColor = (rgb_color){ intensity, intensity, intensity};
      break;  
    default: 
        pixelColor = (rgb_color){ 0, 0 , intensity };
        break;
  }
  return pixelColor;
}
/*
void cleanRing() {
    for(uint32_t i = 0; i < LED_COUNT; i++) {
        colors[i] = (rgb_color){ 0, 0, 0 };
    }
    wristLedStrip.write(colors, LED_COUNT);
}
*/

void pulse(char color, PololuLedStrip led, int led_count) {
    if (counter != 0) { return; };
    rgb_color colors[led_count];
    // Update the colors array.
    for( int j = 0; j < 255; j+=10) {
        for( uint32_t i = 0; i < led_count; i++ ) {
            colors[i] = getColor(color, j);
        }
        led.write(colors, led_count);
    }
    //wait_ms(100);
}

int main(void)
{
    //configure on-board I2C accelerometer on KL25Z
    MMA8451Q acc(SDA, SCL, MMA8451_I2C_ADDRESS); 
    //map read acceleration to PWM output on red status LED
    PwmOut rled(LED_RED);
    float x,y,z;
    
    on_off.attach_asserted(&on_off_pressed);
    on_off.setAssertValue(0); //pins are PullUp, so there activelow buttons.
    on_off.setSampleFrequency(); // Defaults to 20ms.
    
    timer.start();
    timer_begin = timer.read_ms();
    pc.printf("helllo\n");
    //pulse();
    while (true) {
        if (on) {
            x = acc.getAccX();
            y = acc.getAccY();
            z = acc.getAccZ();
            
            if ( x > LEAN_RIGHT_THRESH ) {
                pc.printf("Lean right\n");
                for (int i = 0; i < X_MOVEMENT_REPEATS; i++) {
                    keyboard.putc(RIGHT);
                }
            } else if (x < LEAN_LEFT_THRESH) {
                pc.printf("Lean left\n"); 
                for (int i = 0; i < X_MOVEMENT_REPEATS; i++) {
                    keyboard.putc(LEFT);
                }
            }
            if( y < JUMP_THRESH && timer.read_ms() - timer_begin > 300 ) {                  
                timer_begin = timer.read_ms();
                pc.printf("Jump\n");
                keyboard.putc(ENTER);
                keyboard.putc(SPACE);
            }
            if(wrist.read() < LASER_THRESH) {
                pc.printf("Shoot\n");
                keyboard.putc(LASER);
                pulse('r', wristLedStrip, WRIST_LED_COUNT);
                pulse('r', chestLedStrip, CHEST_LED_COUNT);
            } 
            else if(rightArm.read() < SHIELD_THRESH) {
                pc.printf("Shield\n");   
                keyboard.putc(SHIELD); 
                pulse('g', wristLedStrip, WRIST_LED_COUNT);
                pulse('g', chestLedStrip, CHEST_LED_COUNT);
            } 
            else if ( z > LEAN_BACK_THRESH) {
                pc.printf("Leaning back\n");
                keyboard.putc(SLOW);
                pulse('b', wristLedStrip, WRIST_LED_COUNT);
                pulse('b', chestLedStrip, CHEST_LED_COUNT);
            } else{
                pulse('w', wristLedStrip, WRIST_LED_COUNT);
                pulse('w', chestLedStrip, CHEST_LED_COUNT);
            }
            counter = (counter + 1)%COUNTER_MOD;
        }
    }
}