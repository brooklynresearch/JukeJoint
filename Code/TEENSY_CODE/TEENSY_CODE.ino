/*
 * Still need:
 * 1. Do not play audio if it just turned on on the position of the audio file
 * 2. Reject when audio finished playing.
*/




#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(0, 1);

AudioPlaySdWav           playSdWav1;
AudioFilterStateVariable filter1;        //xy=357,153
AudioMixer4              mixer1;         //xy=561,137
AudioOutputI2S           i2s1;
AudioConnection          patchCord1(playSdWav1, 0, filter1, 0);
AudioConnection          patchCord2(filter1, 0, mixer1, 0);
AudioConnection          patchCord3(filter1, 1, mixer1, 1);
AudioConnection          patchCord4(filter1, 2, mixer1, 2);
AudioConnection          patchCord6(mixer1, 0, i2s1, 0);
AudioControlSGTL5000     sgtl5000_1;

// PINS
const int volumePin = A3;    // VOLUME
const int freqPin = A2;    // VOLUME
const int audioRelay = 4;
const int rejectRelay = 5;
Bounce button3 = Bounce(3, 15); // STOP / PLAY

// SENSOR VALUES
char trackName[11] = {'A', 'C', 'E', 'G', 'J'};
int trackVal[10]   = {110,  268, 428, 594, 755};
const long interval = 3000;
int range = 3;

//SERIAL COM
byte low, high;
int sensorValue;
int pValue = 0;

// AUDIO FILES
int filenumber = 0;
const char * filelist[5] = {
  "Bigwild.WAV", "PerryC.WAV", "ABBA.WAV", "Airsup.WAV", "OL16.WAV"
};

// ADDITIONAL VARS5*-+
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
boolean loopBreak = false;
boolean rejectEndofSong = false;
int playVar = 0;


void setup() { 
  pinMode(3, INPUT_PULLUP);
  pinMode(audioRelay, OUTPUT);
  pinMode(rejectRelay, OUTPUT);
  
  Serial.begin(9600);
  while(!Serial){
    }
    Serial.println("YO");
  mySerial.begin(9600);
  mySerial.println("Yo BRO");
  
  AudioMemory(8);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.7);    // VOLUME, can change to 0.9-1
  SPI.setMOSI(7);
  SPI.setSCK(14);
  
  if (!(SD.begin(10))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
  }}
  
  //mixer1.gain(0, 0.0);  
  //mixer1.gain(1, 0.0);
  mixer1.gain(2, 1.0);  //low-pass signal
  //mixer1.gain(3, 0.0);
  
  delay(1000);
}


int readValue(){
  if (mySerial.available()) {//read
      while(1){
        if(mySerial.read() == '$'){
          delay(1);
          low = mySerial.read();
          delay(1);
          high = mySerial.read();
          break;
        }
      }
      sensorValue = word(high, low);
    }
}

void loop() { 
  if (playSdWav1.isPlaying() == false && playVar == 1) {
    const char *filename = filelist[filenumber];
    playSdWav1.play(filename);
    delay(10);
    playVar = 0;   // DETECT WHEN THE TRACK IS OVER TO REJECT THE RECORD ???????
  }
        
  if(playSdWav1.isPlaying()){  
    button3.update();
    if (button3.fallingEdge()) {
      playSdWav1.stop(); 
      digitalWrite(audioRelay, LOW);
      digitalWrite(rejectRelay, HIGH);
      delay(300);
      digitalWrite(rejectRelay, LOW);
      playVar = 0;   // DETECT WHEN THE TRACK IS OVER TO REJECT THE RECORD ???????
  }}
  
  if(!playSdWav1.isPlaying()){  
    digitalWrite(audioRelay, LOW);
      //digitalWrite(rejectRelay, HIGH);
      //delay(500);
      //digitalWrite(rejectRelay, LOW);
    readValue();
    if(sensorValue > pValue + range || sensorValue < pValue - range )
    {
    for(int i=0; i<6; i++){
      if (sensorValue >= trackVal[i]-range && sensorValue <= trackVal[i]+range) { 
          currentMillis = millis();
          previousMillis = millis(); 
          for(int x = currentMillis - previousMillis; loopBreak == false; x=millis()-previousMillis){
            readValue();
            if(sensorValue < trackVal[i]-range || sensorValue > trackVal[i]+range){
              break;}
            if(sensorValue >= trackVal[i]-range && sensorValue <= trackVal[i]+range){
                if(x >= interval){
                    digitalWrite(audioRelay, HIGH);
                    Serial.println("AUDIO RELAY ON!");
                    previousMillis = millis();
                    int j=i;
                    Serial.print("Play track: ");
                    Serial.print(trackName[j]);
                    Serial.print("; File: ");
                    Serial.print(filelist[j]);
                    Serial.print("; ");
                    playVar = 1;
                    loopBreak = true;
                    playSdWav1.play(filelist[j]);
                    pValue = sensorValue;
  }}}}}} 
  }
  loopBreak = false;
  Serial.print(sensorValue);

  // FREQUENCY ***********
  int knob = analogRead(A2);
  float freq =  expf((float)knob / 150.0) * 10.0 + 80.0;
  filter1.frequency(freq);
  Serial.print("; Frequency = ");
  Serial.println(freq);
  
  // VOLUME***************
  int knob2 = analogRead(volumePin);
  float vol = (float)knob2 / 1280.0;
  sgtl5000_1.volume(vol); // 0-1
  
  delay(100);                  
}
