
// Advanced Microcontroller-based Audio Workshop
//
// https://github.com/PaulStoffregen/AudioWorkshop2015/raw/master/workshop.pdf
// https://hackaday.io/project/8292-microcontroller-audio-workshop-had-supercon-2015
// 
// Part 1-5: Respond to Pushbuttons & Volume Knob
//
// Do more while playing.  Monitor pushbuttons and adjust
// the volume.  Whe the buttons are pressed, stop playing
// the current file and skip to the next or previous.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>
#include <SoftwareSerial.h>

#define DEBUG
//#define DEBUGSENSOR
//#define DEBUGVOLUME
//#define DEBUGFREQUENCY
//#define DEBUGPEAK

#define LEDPIN 20

SoftwareSerial mySerial(0, 1);

AudioPlaySdWav           playSdWav1;
AudioFilterStateVariable filter1;        //xy=357,153
AudioMixer4              mixer1;         //xy=561,137
AudioOutputI2S           i2s1;
AudioAnalyzePeak         peak1;          //xy=317,123
AudioConnection          patchCord1(playSdWav1, 0, filter1, 0);
AudioConnection          patchCord2(filter1, 0, mixer1, 0);
AudioConnection          patchCord3(filter1, 1, mixer1, 1);
AudioConnection          patchCord4(filter1, 2, mixer1, 2);
AudioConnection          patchCord6(mixer1, 0, i2s1, 0);
AudioConnection          patchCord7(playSdWav1, peak1);
AudioControlSGTL5000     sgtl5000_1;

//INPUT PINS
const int rejectButtonPin = 3;
Bounce rejectButton = Bounce(rejectButtonPin, 15); // 15 = 15 ms debounce time
const int volumePin = A3;    // VOLUME
const int freqPin = A2;    // VOLUME

//OUTPUT PINS
const int audioRelay = 4;

const int rejectRelay = 5;
bool isRejectRelayOn = false;
long rejectTiming = 0;
int rejectThreshold = 300;

// SENSOR VALUES
char trackName[11] = {'A', 'C', 'E', 'H', 'J'};
int trackVal[10]   = {110,  268, 428, 689, 755};
//const long interval = 3000;
const long audioPlayThreshold = 5000;
uint32_t trackLengthMillis = 0;
uint32_t currentTrackMillis = 0;
bool audioFilePlayed = false;
int range = 3;

//SERIAL COM
byte low, high;
int sensorValue;
int pValue = 0;

// AUDIO FILES
int fileNumber = 0;  //file to play
#define TRACKTOTAL 5
const char * filelist[5] = {
  "Bigwild.WAV", "PerryC.WAV", "ABBA.WAV", "Airsup.WAV", "OL16.WAV"
};
bool firstTimeThrough[TRACKTOTAL] = {true, true, true, true, true};
bool isValidReading[TRACKTOTAL] = {false, false, false, false, false};

// ADDITIONAL VARS5*-+
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
boolean loopBreak = false;
boolean rejectEndofSong = false;
bool playVar = false;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  
  AudioMemory(8);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  SPI.setMOSI(7);
  SPI.setSCK(14);
  if (!(SD.begin(10))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  pinMode(13, OUTPUT); // LED on pin 13
  pinMode(rejectButtonPin, INPUT_PULLUP);
  pinMode(rejectRelay, OUTPUT);

  pinMode(audioRelay, OUTPUT);

  pinMode(LEDPIN, OUTPUT);
  
  delay(1000);
}


void loop() {
  
  if (!playSdWav1.isPlaying() && playVar) {
    Serial.print("We're Playing Track: ");
    Serial.println(filelist[fileNumber]);
    const char *fileName = filelist[fileNumber];
    playSdWav1.play(fileName);
    delay(10);
    trackLengthMillis = playSdWav1.lengthMillis();
    currentTrackMillis = millis();
    Serial.print("Length of File: ");Serial.println(playSdWav1.lengthMillis());
    muteAudio();
    //firstTimeThrough[fileNumber] = true;
    isValidReading[fileNumber] = false;
    playVar = false;   // DETECT WHEN THE TRACK IS OVER TO REJECT THE RECORD ???????
  } else if(playSdWav1.isPlaying()){
    //Serial.print("POS: ");Serial.println(playSdWav1.positionMillis());
    digitalWrite(audioRelay, HIGH);
    if(peak1.available()){
      int currentPeak = peak1.read() * 255;
      analogWrite(LEDPIN, currentPeak);
      #ifdef DEBUGPEAK
      Serial.println(currentPeak);
      #endif
    }
    audioFilePlayed = true;
    unMuteAudio();
  } else {
    analogWrite(LEDPIN, 0);
    if(audioFilePlayed && (millis() - currentTrackMillis > trackLengthMillis)){
      Serial.println("Custom Audio File Ended");
      isRejectRelayOn = true;
      rejectTiming = millis();
      audioFilePlayed = false;
    }
    digitalWrite(audioRelay, LOW);
  }

  

  // CHECK SENSOR READING
  checkSensorValue();

  // CHECK VALID READING *
  checkToPlayAudio();
  
  // REJECT POLLING ******
  pollRejectButton();

  // FREQUENCY ***********
  setFrequency();
  
  // VOLUME **************
  setVolume();

  // UPDATE REJECT STATE *
  isRejectRelayOn = pulseRelay(rejectRelay, isRejectRelayOn, rejectTiming, rejectThreshold);
}
void checkSensorValue(){
  sensorValue = readValue();
  if(sensorValue > 0){
    
    #ifdef DEBUGSENSOR
    Serial.print("SENSOR VALUE: ");
    Serial.println(sensorValue);
    #endif

    for(int i=0; i<TRACKTOTAL; i++){
        if(sensorValue >= (trackVal[i] - range) && sensorValue <= (trackVal[i] + range)){
          if(firstTimeThrough[i]){
            Serial.print("MIGHT PLAY TRACK: ");Serial.println(filelist[i]);
            isValidReading[i] = true;
            fileNumber = i;
            currentMillis = millis();
            firstTimeThrough[i] = false;
          }
        } else {
          //Serial.println("RESET PARAMS");
          firstTimeThrough[i] = true;
          isValidReading[i] = false;
        }
      } 
    }
}
void checkToPlayAudio(){
  for(int i=0; i<TRACKTOTAL; i++){
   if(isValidReading[i] && (millis() - currentMillis > audioPlayThreshold)){
      Serial.print("PLAYING TRACK: ");Serial.println(filelist[fileNumber]);
      playVar = true;
   }
  }
}

void setFrequency(){
  int knob = analogRead(A2);
  float freq =  expf((float)knob / 150.0) * 10.0 + 80.0;
  filter1.frequency(freq);
  #ifdef DEBUGFREQUENCY
  Serial.print("Frequency: ");
  Serial.print(freq);  
  #endif
}

void setVolume(){
  int knob2 = analogRead(volumePin);
  float vol = (float)knob2 / 1280.0;
  sgtl5000_1.volume(vol); // 0-1
  #ifdef DEBUGVOLUME
  Serial.print("\t Volume: ");
  Serial.println(vol);
  #endif
}

void pollRejectButton(){
  rejectButton.update();
  if (rejectButton.fallingEdge()) {
    #ifdef DEBUG
    Serial.println("REJECTING");
    #endif
    playSdWav1.stop();
    muteAudio();
    digitalWrite(audioRelay, LOW);
    isRejectRelayOn = true;
    rejectTiming = millis();
    playVar = 0;
    unMuteAudio();
  }
}

int readValue(){
  if (mySerial.available() > 0) {//read
      //while(1){
        if(mySerial.read() == '$'){
          //delay(1);
          low = mySerial.read();
          //delay(1);
          high = mySerial.read();
          //break;
        }
      //}
      int sensorReading = word(high, low);
      return sensorReading;
    }
}

bool pulseRelay(int whichRelay, bool pulsing, long timer, int timeThreshold){
  if(pulsing){
    if(millis() - timer < timeThreshold){
      digitalWrite(whichRelay, HIGH);
      return pulsing;
    } else {
      digitalWrite(whichRelay, LOW);
      pulsing = false;
      return pulsing;
    }
  }
}

void muteAudio(){
    sgtl5000_1.muteLineout();
}

void unMuteAudio(){
    sgtl5000_1.unmuteLineout();
}

