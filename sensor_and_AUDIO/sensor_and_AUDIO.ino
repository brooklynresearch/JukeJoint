#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>

#define AVERAGE 10

AudioPlaySdWav           playSdWav1;
AudioOutputI2S           i2s1;
AudioConnection          patchCord1(playSdWav1, 0, i2s1, 0);
AudioConnection          patchCord2(playSdWav1, 1, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;

// PINS
int volumePin = A3;    // VOLUME
int sensorPin = A2;    // select the input pin for the potentiometer
Bounce button1 = Bounce(1, 15); // STOP / PLAY

// SENSOR VALUES
char trackName[11] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K'};
int trackVal[10]   = {70,  123, 176, 229, 283, 338, 394, 448, 502, 555};
const long interval = 3000;
int range = 5;

// AUDIO FILES
int filenumber = 0;
const char * filelist[5] = {
  "Bigwild.WAV", "PerryC.WAV", "ABBA.WAV", "Airsup.WAV", "OL16.WAV"
};

// ADDITIONAL VARS
int sensorValue = 0;
long semiRawValue = 0;
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
boolean loopBreak = false;
int playVar = 0;


void setup() { 
  Serial.begin(9600);
  AudioMemory(8);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);    // VOLUME, can change to 0.9-1
  SPI.setMOSI(7);
  SPI.setSCK(14);
  if (!(SD.begin(10))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
  }}
  pinMode(1,  INPUT_PULLUP);
  delay(1000);
}

void loop() { 
  if (playSdWav1.isPlaying() == false && playVar == 1) {
    const char *filename = filelist[filenumber];
    playSdWav1.play(filename);
    delay(10);
    playVar = 0;   // DETECT WHEN THE TRACK IS OVER TO REJECT THE RECORD ???????
  }
  if(playSdWav1.isPlaying()){
    button1.update();
    if (button1.fallingEdge()) {
      playSdWav1.stop(); 
      playVar = 0;   // DETECT WHEN THE TRACK IS OVER TO REJECT THE RECORD ???????
  }}
  
// AVERAGE SENSOR READINGS
  sensorValue = 0;
  for(int i=0; i<AVERAGE; i++){
    semiRawValue = 0;
    for(int j=0; j<AVERAGE; j++){
      semiRawValue += analogRead(sensorPin);
    }
    sensorValue += (semiRawValue / AVERAGE);
  }
  sensorValue /= AVERAGE;
  
  if(!playSdWav1.isPlaying()){
    for(int i=0; i<11; i++){
      if (sensorValue >= trackVal[i]-range && sensorValue <= trackVal[i]+range) { 
          currentMillis = millis();
          previousMillis = millis(); 
          for(int x = currentMillis - previousMillis; loopBreak == false; x=millis()-previousMillis){
            sensorValue = analogRead(sensorPin); 
            if(sensorValue < trackVal[i]-range || sensorValue > trackVal[i]+range){
              break;}
            if(sensorValue >= trackVal[i]-range && sensorValue <= trackVal[i]+range){
                if(x >= interval){
                    previousMillis = millis();
                    int j = (i == 1 || i == 3 || i == 5 || i == 7 || i == 9) ? (i-1)/2 : i; //for A-B,C-D,E-F,G-H,J-K to be the same tracks
                    Serial.print("Play track: ");
                    Serial.print(trackName[j]);
                    Serial.print("; File: ");
                    Serial.print(filelist[j]);
                    Serial.print("; ");
                    playVar = 1;
                    loopBreak = true;
                    playSdWav1.play(filelist[j]);
  }}}}}} 
  loopBreak = false;
  Serial.print("Sensor Value: ");
  Serial.print(sensorValue);
  Serial.println(); 
  
  // VOLUME***************
  int knob = analogRead(volumePin);
  float vol = (float)knob / 1280.0;
  sgtl5000_1.volume(vol); // 0-1
  
  delay(100);                  
}
