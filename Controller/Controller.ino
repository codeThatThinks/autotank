#include <jm_CPPM.h>

/*
 * RECEIVER CHANNEL MAPPING:
 * - ELEV: THROTTLE
 * - AILE: STERRING
 * - AUX1: ARM
 */
 

/**** HARDWARE CONFIG ****/
const int LEFT_TRACK_PIN = 6;
const int RIGHT_TRACK_PIN = 5;
const int SPARE_PIN = 3;

/**** RECEIVER CONFIG ****/
const int PPM_MIN_VAL = 1000;         // microsecond value for min stick
const int PPM_CENTER_VAL = 1500;      // microsecond value for center stick
const int PPM_MAX_VAL = 2000;         // microsecond value for max stick
const int PPM_DEADZONE = 12;          // tight stick deadzone to prevent oscillating inputs
const int ARM_MIN_VAL = 1900;         // minimum value in order to arm

/**** MOTOR CONFIG ****/
const int PWM_MIN_VAL = 0;
const int PWM_MAX_VAL = 255;

/**** MIXING CONFIG ****/
const int THROTTLE_DEADZONE = 50;     // +/- deadzone from center stick
const int STEERING_DEADZONE = 50;     // +/- deadzone from center stick

/**** GLOBAL VARIABLES ****/
bool armed = false;
int ch_arm = PPM_MIN_VAL;             // aux1 channel
int ch_throttle = PPM_CENTER_VAL;     // elevator channel
int ch_steering = PPM_CENTER_VAL;     // aileron channel
int left_track_val = PPM_CENTER_VAL;
int right_track_val = PPM_CENTER_VAL;
int led_val = 0;
int desync_counter = 0;

int throttle_delta = 0;
int steering_delta = 0;

void setup() {
  pinMode(LEFT_TRACK_PIN, OUTPUT);
  pinMode(RIGHT_TRACK_PIN, OUTPUT);
  pinMode(SPARE_PIN, OUTPUT);
  pinMode(13, OUTPUT);
    
  CPPM.begin();
  Serial.begin(115200);
}

void loop() {
  arm_disarm();
  read_receiver();
  if(armed) mix_motors();
  write_motors();
  led_status();

  Serial.print(armed);
  Serial.print(",");
  Serial.print(ch_arm);
  Serial.print(",");
  Serial.print(ch_throttle);
  Serial.print(",");
  Serial.print(ch_steering);
  Serial.print(",");
  Serial.print(left_track_val);
  Serial.print(",");
  Serial.print(right_track_val);
  Serial.print("\n");
}


// Read channels from receiver and update global variables
void read_receiver() {
  CPPM.cycle();
  if(CPPM.synchronized()) {
    ch_arm = CPPM.read_us(CPPM_AUX1);
    ch_throttle = CPPM.read_us(CPPM_ELEV);
    ch_steering = CPPM.read_us(CPPM_AILE);

    // stick deadzone
    if(ch_throttle >= (PPM_CENTER_VAL - PPM_DEADZONE)
       && ch_throttle <= (PPM_CENTER_VAL + PPM_DEADZONE)) {
      ch_throttle = PPM_CENTER_VAL;
    }

    if(ch_steering >= (PPM_CENTER_VAL - PPM_DEADZONE)
       && ch_steering <= (PPM_CENTER_VAL + PPM_DEADZONE)) {
      ch_steering = PPM_CENTER_VAL;
    }
  }
}

// Arming/disarming logic
void arm_disarm() {
  if(CPPM.synchronized() 
     && ch_arm >= ARM_MIN_VAL
     && ch_throttle >= (PPM_CENTER_VAL - PPM_DEADZONE)
     && ch_throttle <= (PPM_CENTER_VAL + PPM_DEADZONE)
     && ch_steering >= (PPM_CENTER_VAL - PPM_DEADZONE)
     && ch_steering <= (PPM_CENTER_VAL + PPM_DEADZONE)) {
      armed = true;
      desync_counter = 0;
  } else if(armed
            && !(CPPM.synchronized())) {
        desync_counter++;
  } else if(ch_arm < ARM_MIN_VAL
            || (armed
                && desync_counter >= 5000)) {
    armed = false;
  }
}

// Led on pin 13 for armed status
void led_status() {
  if(armed) {
    led_val = ~led_val & 0x1;
    digitalWrite(13, led_val);
  } else {
    digitalWrite(13, LOW);
  }
}

// Calculate and set speeds for each track
void mix_motors() {
  if(!armed) {
    left_track_val = PPM_CENTER_VAL;
    right_track_val = PPM_CENTER_VAL;
    return;
  }

  throttle_delta = ch_throttle - PPM_CENTER_VAL;
  steering_delta = ch_steering - PPM_CENTER_VAL;
  
  if(throttle_delta >= THROTTLE_DEADZONE) {
    if(steering_delta > -STEERING_DEADZONE
       && steering_delta < STEERING_DEADZONE) {
      // forward
      left_track_val = map(throttle_delta, THROTTLE_DEADZONE, PPM_MAX_VAL - PPM_CENTER_VAL, PPM_CENTER_VAL, PPM_MAX_VAL);
      right_track_val = left_track_val;
      
    } else if(steering_delta <= -STEERING_DEADZONE) {
      // forward, left turn
      right_track_val = map(throttle_delta, THROTTLE_DEADZONE, PPM_MAX_VAL - PPM_CENTER_VAL, PPM_CENTER_VAL, PPM_MAX_VAL);
      left_track_val = right_track_val - map(steering_delta, -STEERING_DEADZONE, PPM_MIN_VAL - PPM_CENTER_VAL, 0, right_track_val - PPM_CENTER_VAL);
      
    } else if(steering_delta >= STEERING_DEADZONE) {
      // forward, right turn
      left_track_val = map(throttle_delta, THROTTLE_DEADZONE, PPM_MAX_VAL - PPM_CENTER_VAL, PPM_CENTER_VAL, PPM_MAX_VAL);
      right_track_val = left_track_val - map(steering_delta, STEERING_DEADZONE, PPM_MAX_VAL - PPM_CENTER_VAL, 0, left_track_val - PPM_CENTER_VAL);
    
    }
  } else if(throttle_delta <= -THROTTLE_DEADZONE) {
    if(steering_delta > -STEERING_DEADZONE
       && steering_delta < STEERING_DEADZONE) {
      // reverse
      left_track_val = map(throttle_delta, -THROTTLE_DEADZONE, PPM_MIN_VAL - PPM_CENTER_VAL, PPM_CENTER_VAL, PPM_MIN_VAL);
      right_track_val = left_track_val;
      
    } else if(steering_delta <= -STEERING_DEADZONE) {
      // reverse, left turn
      right_track_val = map(throttle_delta, -THROTTLE_DEADZONE, PPM_MIN_VAL - PPM_CENTER_VAL, PPM_CENTER_VAL, PPM_MIN_VAL);
      left_track_val = right_track_val + map(steering_delta, -STEERING_DEADZONE, PPM_MIN_VAL - PPM_CENTER_VAL, 0, PPM_CENTER_VAL - right_track_val);
      
    } else if(steering_delta >= STEERING_DEADZONE) {
      // reverse, right turn
      left_track_val = map(throttle_delta, -THROTTLE_DEADZONE, PPM_MIN_VAL - PPM_CENTER_VAL, PPM_CENTER_VAL, PPM_MIN_VAL);
      right_track_val = left_track_val + map(steering_delta, STEERING_DEADZONE, PPM_MAX_VAL - PPM_CENTER_VAL, 0, PPM_CENTER_VAL - left_track_val);
    
    }
  } else {
    // stopped
    left_track_val = PPM_CENTER_VAL;
    right_track_val = PPM_CENTER_VAL;
    
  }
}

// Write pwm value to motors
void write_motors() {
  analogWrite(LEFT_TRACK_PIN, map(left_track_val, PPM_MIN_VAL, PPM_MAX_VAL, PWM_MIN_VAL, PWM_MAX_VAL));
  analogWrite(RIGHT_TRACK_PIN, map(right_track_val, PPM_MIN_VAL, PPM_MAX_VAL, PWM_MIN_VAL, PWM_MAX_VAL));
}
