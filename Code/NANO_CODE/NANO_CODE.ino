#define AVERAGE 10

// SENSOR VALUE
int sensorPin = A2;
int sensorValue = 0;
long semiRawValue = 0;

void setup()  
{
  Serial.begin(9600);
  while (!Serial) {
  }
}

void loop()
{
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
    Serial.print('$');
    Serial.print(char(lowByte(sensorValue)));
    Serial.print(char(highByte(sensorValue)));
    //Serial.println(sensorValue);
  delay(100);
}
