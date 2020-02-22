import processing.serial.*;

int deadzone = 50;

Serial myPort;
boolean armed = false;
int ch_arm = 1000;
int ch_throttle = 1500;
int ch_steering = 1500;
int left_track_val = 1500;
int right_track_val = 1500;

void setup () {
  myPort = new Serial(this, "/dev/ttyACM0", 115200);
  myPort.bufferUntil('\n');

  size(1280, 1280);
  //delay(1000);
}

void bar(float x, float y, float val, String label) {
  fill(0, 0, 0);
  textSize(24);
  textAlign(CENTER);
  text(label, x, y - 110);
  text(int(val), x, y + 130);
  
  noFill();
  stroke(0, 0, 0);
  rect(x - 21, y - 101, 41, 201);
  noStroke();
  if(val > 1500) {
    fill(0, 200, 0);
    rect(x - 20, y - map(val, 1500, 2000, 0, 100), 40, map(val, 1500, 2000, 0, 100));
  } else if(val < 1500) {
    fill(255, 0, 0);
    rect(x - 20, y, 40, map(val, 1500, 1000, 0, 100));
  }
}

void draw () {
  background(255, 255, 255);
  
  int bound = (height < width) ? height : width;
  int xzero = (width - bound) / 2;
  int yzero = (height - bound) / 2;
  int xmax = xzero + bound;
  int ymax = yzero + bound;
  
  strokeWeight(1);
  
  fill(0, 0, 0);
  textSize(28);
  textAlign(CENTER);
  text("AUTOTANK DEBUG TOOL", xzero + bound / 2, yzero + 65);
  
  // draw axes
  stroke(200, 200, 200);
  line(xzero, height / 2, xmax, height / 2);
  line(width / 2, yzero, width / 2, ymax);
  
  // draw arm state
  textAlign(CENTER);
  if(armed) {
    fill(0, 200, 0);
    noStroke();
    rect(xzero + 50, ymax - 100, 200, 50);
    fill(255, 255, 255);
    text("ARMED", xzero + 150, ymax - 65);
  } else {
    fill(255, 0, 0);
    noStroke();
    rect(xzero + 50, ymax - 100, 200, 50);
    fill(255, 255, 255);
    text("DISARMED", xzero + 150, ymax - 65);
  }
  
  // draw channels
  bar(xzero + 80, yzero + 175, ch_arm, "ARM");
  bar(xzero + 180, yzero + 175, ch_throttle, "THRO");
  bar(xzero + 280, yzero + 175, ch_steering, "STEER");
  
  // draw track values
  bar(xmax - 180, yzero + 175, left_track_val, "LEFT");
  bar(xmax - 80, yzero + 175, right_track_val, "RIGHT");
  
  // draw input
  noFill();
  stroke(0, 0, 200);
  strokeWeight(3);
  circle(xzero + map(ch_steering, 1000, 2000, 0, bound), ymax - map(ch_throttle, 1000, 2000, 0, bound), 35);
}

void serialEvent (Serial myPort) {
  try {
    String inString = myPort.readStringUntil('\n');
    
    if (inString != null) {
      String[] vals = split(trim(inString), ',');
      if(vals.length == 6) {
        armed = (int(vals[0]) == 1);
        ch_arm = int(vals[1]);
        ch_throttle = int(vals[2]);
        ch_steering = int(vals[3]);
        left_track_val = int(vals[4]);
        right_track_val = int(vals[5]);
      }
    }
  }
  catch(RuntimeException e) {
    e.printStackTrace();
  }
}
