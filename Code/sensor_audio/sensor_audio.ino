#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>

#define AVERAGE 10

AudioPlaySdWav           playSdWav1;
AudioFilterStateVariable filter1;        //xy=357,153
AudioMixer4              mixer1;         //xy=561,137
AudioOutputI2S           i2s1;
//AudioConnection          patchCord1(playSdWav1, 0, i2s1, 0);
AudioConnection          patchCord1(playSdWav1, 0, filter1, 0);
AudioConnection          patchCord2(filter1, 0, mixer1, 0);
AudioConnection          patchCord3(filter1, 1, mixer1, 1);
AudioConnection          patchCord4(filter1, 2, mixer1, 2);
AudioConnection          patchCord6(mixer1, 0, i2s1, 0);
AudioControlSGTL5000     sgtl5000_1;

// PINS
const int volumePin = A3;    // VOLUME
const int freqPin = A2;    // VOLUME
const int sensorPin = A4;    // select the input pin for the potentiometer
const int audioRelay = 4;

Bounce button0 = Bounce(0, 15);
Bounce button1 = Bounce(1, 15);  // 15 ms debounce time
Bounce button2 = Bounce(2, 15);
Bounce button3 = Bounce(3, 15); // STOP / PLAY

// SENSOR VALUES
char trackName[11] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K'};
int trackVal[10]   = {588,  712, 778, 828, 861, 882, 905, 921, 932, 953};
//char trackName[11] = {'A', 'C', 'E', 'G', 'J'};
//int trackVal[10]   = {581,  783, 861, 905, 936};
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
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(audioRelay, OUTPUT);
  Serial.begin(9600);
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
  mixer1.gain(0, 0.0);  
  mixer1.gain(1, 0.0);  // default to hearing band-pass signal
  mixer1.gain(2, 1.0);
  mixer1.gain(3, 0.0);
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
    button0.update();
    button1.update();
    button2.update();
    
    if (button0.fallingEdge()) {
      Serial.println("Low Pass Signal");
      mixer1.gain(0, 1.0);  // hear low-pass signal
      mixer1.gain(1, 0.0);
      mixer1.gain(2, 0.0);
    }
    if (button1.fallingEdge()) {
      Serial.println("Band Pass Signal");
      mixer1.gain(0, 0.0);
      mixer1.gain(1, 1.0);  // hear low-pass signal
      mixer1.gain(2, 0.0);
    }
    if (button2.fallingEdge()) {
      Serial.println("High Pass Signal");
      mixer1.gain(0, 0.0);
      mixer1.gain(1, 0.0);
      mixer1.gain(2, 1.0);  // hear low-pass signal
    }
    
    button3.update();
    if (button3.fallingEdge()) {
      playSdWav1.stop(); 
      digitalWrite(audioRelay, LOW);
      Serial.println("AUDIO RELAY OFF!");
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
    digitalWrite(audioRelay, LOW);
    Serial.println("AUDIO RELAY OFF!");
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
                    digitalWrite(audioRelay, HIGH);
                    Serial.println("AUDIO RELAY ON!");
                    previousMillis = millis();
                    int j = (i == 1 || i == 3 || i == 5 || i == 7 || i == 9) ? (i-1)/2 : i/2; //for A-B,C-D,E-F,G-H,J-K to be the same tracks
                    //int j=i;
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
  //Serial.println(); 

  // FREQUENCY ***********
  // read the knob and adjust the filter frequency
  int knob = analogRead(A2);
  // quick and dirty equation for exp scale frequency adjust
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
