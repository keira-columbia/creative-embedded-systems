import processing.serial.*;

// Your Turn: choose a place, choose a movement, and add it to your dance.
Serial port;
String incoming = "";
String[] moves = {"Plié", "Turn", "Jump"};
int lastAttempt = -3000, lastPacket = -10000;
float stickX = 2048, stickY = 2048, knob = 0;
int rawButton = 1, stableButton = 1, buttonChanged = 0;
boolean firstPacket = true;
int selected = 0;
float markerX = 540, markerY = 460;
float dancerX = 540, dancerY = 460, fromX, fromY;
float[] stepX = new float[4], stepY = new float[4];
int[] stepMove = new int[4];
int count = 0, playing = 0, phaseStart = 0;
// 0: choose, 1: preview added step, 2: playback, 3: finished
int phase = 0;
int previousFrame;
boolean demo = false;

void settings() {
  fullScreen();
  pixelDensity(1);
  smooth(4);
}

void setup() {
  surface.setTitle("Your Turn — Ballet Studio");
  textFont(createFont("Arial", 18));
  previousFrame = millis();
}

void draw() {
  // Fit the original stage to the screen without stretching the dancer.
  float stageScale = min(width/1080.0, height/760.0);
  float offsetX = (width-1080*stageScale)/2;
  float offsetY = (height-760*stageScale)/2;
  readController();
  float dt = min((millis()-previousFrame)/1000.0, 0.05);
  previousFrame = millis();
  boolean live = millis()-lastPacket < 1200;
  if (phase == 0 && (live || demo)) {
    if (demo) {
      markerX = constrain((mouseX-offsetX)/stageScale, 180, 900);
      markerY = constrain((mouseY-offsetY)/stageScale, 365, 535);
    } else {
      markerX = constrain(markerX + axis(stickX)*260*dt, 180, 900);
      markerY = constrain(markerY + axis(stickY)*150*dt, 365, 535);
      // Hysteresis keeps a knob near a boundary from flickering between steps.
      if (selected < 2 && knob > (selected+1)*1365+90) selected++;
      if (selected > 0 && knob < selected*1365-90) selected--;
    }
  }
  float progress = 0;
  int movement = selected;
  if (phase == 1 || phase == 2) {
    int i = phase == 1 ? count-1 : playing;
    float elapsed = millis()-phaseStart;
    float travel = constrain(elapsed/650, 0, 1);
    float eased = travel*travel*(3-2*travel);
    dancerX = lerp(fromX, stepX[i], eased);
    dancerY = lerp(fromY, stepY[i], eased);
    progress = constrain((elapsed-650)/1550, 0, 1);
    movement = stepMove[i];
    if (elapsed > 2500) {
      if (phase == 1) {
        if (count == 4) startPlayback();
        else phase = 0;
      } else if (++playing < count) beginAnimation();
      else { phase = 3; phaseStart = millis(); }
    }
  }
  pushMatrix();
  translate(offsetX, offsetY);
  scale(stageScale);
  drawStage(live);
  drawDancer(dancerX, dancerY, movement, progress);
  drawChoices();
  popMatrix();
}

float axis(float value) {
  float normalized = (value-2048)/2048.0;
  if (abs(normalized)<0.15) return 0;
  return (normalized>0 ? 1 : -1)*(abs(normalized)-0.15)/0.85;
}

void readController() {
  if (port == null && millis()-lastAttempt>2000) {
    lastAttempt=millis();
    for (String name: Serial.list()) {
      if (name.startsWith("/dev/cu.wchusbserial")) {
        try { port=new Serial(this,name,9600); incoming=""; firstPacket=true; }
        catch(Exception e) { port=null; }
        break;
      }
    }
  }
  if (port == null) return;
  try {
    while (port.available()>0) {
      int c=port.read();
      if (c==10) {
        String[] values=split(trim(incoming),','); incoming="";
        if(values.length!=5) continue;
        try {
          int[] n=new int[5];
          for(int i=0;i<5;i++) n[i]=Integer.parseInt(values[i]);
          if(n[0]<0 || n[0]>4095 || n[1]<0 || n[1]>4095 || n[3]<0 || n[3]>4095) continue;
          stickX=n[0]; stickY=n[1];
          knob=firstPacket ? n[3] : lerp(knob,n[3],0.25);
          if(firstPacket) {
            rawButton=stableButton=n[4]; buttonChanged=millis();
            selected=constrain(int(knob/1365),0,2); firstPacket=false;
          }
          if(n[4]!=rawButton) { rawButton=n[4]; buttonChanged=millis(); }
          if(rawButton!=stableButton && millis()-buttonChanged>=45) {
            stableButton=rawButton;
            if(stableButton==0 && !demo) addStep();
          }
          lastPacket=millis();
        } catch(NumberFormatException e) { }
      } else if(c>=0) {
        incoming+=char(c);
        if(incoming.length()>120) incoming="";
      }
    }
    if(millis()-lastPacket>4500 && millis()-lastAttempt>4500) disconnect();
  } catch(Exception e) { disconnect(); }
}

void disconnect() {
  try { port.stop(); } catch(Exception e) { }
  port=null; incoming="";
}

void addStep() {
  if(phase!=0 || count>=4) return;
  stepX[count]=markerX; stepY[count]=markerY; stepMove[count]=selected;
  count++; phase=1; beginAnimation();
}
void beginAnimation() {
  fromX=dancerX; fromY=dancerY; phaseStart=millis();
}
void startPlayback() { playing=0; phase=2; beginAnimation(); }
void keyPressed() {
  if((key=='d' || key=='D') && count==0 && phase==0) demo=!demo;
  if(demo && key>='1' && key<='3') selected=key-'1';
  if(demo && key==' ') addStep();
}

void drawStage(boolean live) {
  background(249,242,237);
  noStroke(); fill(87,42,66); textSize(42); text("Your Turn",48,65);
  fill(110,88,98);textSize(16);text("A little choreography, made by you.",50,94);
  textAlign(RIGHT);textSize(14);
  fill(demo ? color(150,100,30) : live ? color(40,125,90) : color(180,75,65));
  text(demo ? "DEMO · mouse + 1/2/3 + space" : live ? "CONTROLLER CONNECTED" : "WAITING FOR CONTROLLER",1030,57);
  textAlign(LEFT);
  fill(45,31,53);rect(45,125,990,450,12);
  // Backdrop and curtains frame the stage without hiding the dancer.
  fill(69,47,75);rect(130,145,820,240);
  fill(110,51,78);rect(45,125,85,450,12);rect(950,125,85,450,12);
  stroke(135,68,92);strokeWeight(2);
  for(int i=60;i<125;i+=18) line(i,145,i,550);
  for(int i=960;i<1030;i+=18) line(i,145,i,550);
  noStroke();fill(194,151,127);quad(130,350,950,350,950,575,130,575);
  stroke(166,125,108);strokeWeight(1);
  for(int y=370;y<575;y+=35) line(130,y,950,y);
  for(int x=130;x<=950;x+=82) line(540+(x-540)*0.6,350,x,575);
  // Follow the dancer during playback and keep the light on her at the end.
  float lightX = phase >= 2 ? dancerX : markerX;
  float lightY = phase >= 2 ? dancerY : markerY;
  noStroke();fill(255,238,194,45);triangle(lightX,155,lightX-90,lightY,lightX+90,lightY);
  fill(255,238,194,100);ellipse(lightX,lightY,140,40);
  noFill();stroke(255,241,210);strokeWeight(2);ellipse(lightX,lightY,90,25);
  noStroke();fill(255,239,224);textAlign(CENTER);textSize(18);
  String caption = phase==0 ? "Choose a place and a movement" : phase==1 ? "Step "+count+" added" : phase==2 ? "Your dance · step "+(playing+1)+" of 4" : "Take a bow! Close and rerun the sketch for a new dance.";
  text(caption,540,160);textAlign(LEFT);
}

void drawChoices() {
  textSize(15);fill(100,75,85);text("JOYSTICK  Move the spotlight     KNOB  Choose a step     BUTTON  Add it",50,608);
  for(int i=0;i<3;i++) {
    fill(i==selected ? color(108,55,87) : color(233,218,222));
    rect(50+i*175,625,160,43,10);
    fill(i==selected ? color(255) : color(85,55,70));textSize(20);text(moves[i],68+i*175,653);
  }
  for(int i=0;i<4;i++) {
    fill(phase==2 && i==playing ? color(239,191,127) : color(237,225,215));
    rect(610+i*105,625,95,66,9);fill(91,62,74);textSize(13);
    text("STEP "+(i+1),623+i*105,647);textSize(17);
    text(i<count ? moves[stepMove[i]] : "—",623+i*105,675);
  }
  fill(120,99,105);textSize(13);text("Four choices make a dance. Close and rerun to start again. · D: keyboard demo before starting",50,725);
}

void drawDancer(float x,float y,int move,float t) {
  float wave=sin(PI*t);
  float bend=move==0 ? 23*wave : 0;
  float jump=move==2 ? 80*wave : 0;
  float turn=move==1 ? cos(TWO_PI*t) : 1;
  noStroke();fill(50,25,45,45);ellipse(x,y,55-20*wave*(move==2?1:0),14);
  pushMatrix();translate(x,y-jump);scale(map(y,365,535,0.86,1.08),map(y,365,535,0.86,1.08));
  // Keep a little width during a turn so the figure does not disappear.
  scale(move==1 ? (turn<0?-1:1)*max(0.20,abs(turn)) : 1,1);
  float hip=-55+bend;
  stroke(244,204,184);strokeWeight(8);strokeCap(ROUND);
  if(move==2 && wave>0.05) {
    line(-7,hip,-24-26*wave,-25);line(-24-26*wave,-25,-35-35*wave,-10);
    line(7,hip,24+26*wave,-25);line(24+26*wave,-25,35+35*wave,-10);
  } else {
    line(-7,hip,-13-bend*0.6,-27);line(-13-bend*0.6,-27,-13,0);
    line(7,hip,13+bend*0.6,-27);line(13+bend*0.6,-27,13,0);
  }
  stroke(215,142,166);strokeWeight(6);line(-13,0,-24,2);line(13,0,24,2);
  noStroke();fill(166,79,121);quad(-14,hip-48,14,hip-48,11,hip,-11,hip);
  fill(235,164,193);triangle(0,hip-12,-38,hip+12,38,hip+12);
  fill(249,191,212,150);ellipse(0,hip+5,78,18);
  stroke(244,204,184);strokeWeight(7);
  float lift=move==1 ? 1 : move==2 ? wave : 0;
  line(-14,hip-43,-34,hip-35-35*lift);line(-34,hip-35-35*lift,-45+28*lift,hip-48-48*lift);
  line(14,hip-43,34,hip-35-35*lift);line(34,hip-35-35*lift,45-28*lift,hip-48-48*lift);
  line(0,hip-49,0,hip-58);
  noStroke();fill(78,45,43);ellipse(0,hip-76,29,33);ellipse(13,hip-90,15,15);
  fill(244,204,184);ellipse(0,hip-72,24,27);
  fill(78,45,43);arc(0,hip-80,26,20,PI,TWO_PI);
  fill(80,46,51);ellipse(-5,hip-73,2,2);ellipse(5,hip-73,2,2);
  popMatrix();
}
