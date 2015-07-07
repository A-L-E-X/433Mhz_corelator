
// КОРРЕЛЯТОР

typedef struct trit
{
  uint8_t state;
  uint16_t length;
};

const uint16_t maxElements = 511;
const uint8_t recPin = 2;
const uint8_t RFPin = 12;
//const uint8_t length = 24;
//char array[] = {'I','0','F','F','F','0','0','1','1','0','F','F','0','I','0','F','F','F','0','0','1','0','1','0'};
trit rec[maxElements];


//uint16_t lengths[50] = { 204, 548, 208, 548, 208, 540, 212, 544, 212, 544, 592, 172, 196, 556, 576, 180, 200, 560, 568, 192, 184, 568, 192, 556, 200, 564, 192, 560, 572, 188, 572, 188, 184, 568, 188, 564, 192, 564, 196, 564, 192, 556, 200, 556, 576, 180, 572, 192, 176, 5912 };
//uint8_t states[50]  =  {   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,    0 };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - S E T U P   F U N C T I O N - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void setup()
{
  Serial.begin(115200);
  Serial.println();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - M A I N   L O O P - - - - - - - - - - - - - - - - - - - - - - -

void loop()
{
  Serial.println("begin scaning");// in 2 sec....");
  delay(2000);
  pinScan(recPin);
  Serial.println("waiting....");
  detectCommand(maxElements);
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - 

void replaySequence(uint8_t thePin)
{
  pinMode(thePin, OUTPUT);
  for(uint16_t i=0; i<maxElements; i++)
  {
    digitalWrite(thePin, rec[i].state);
    delayMicroseconds(rec[i].length);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - 

void detectCommand(uint16_t numOfElements)
{
  // find the command code
  // detect the command sequence at least twice
  uint16_t startIndex = 0;
  for(uint16_t i =0; i<numOfElements; i++)
  {
    if(rec[i].length > 31*120) // min is 3720 us for shortest period of 120us
    {
      startIndex = i;
      break;
    }
  }
  if(!(rec[startIndex + 50].length > 31*120))
  {
    Serial.println("NO sequence detected");
    return;
  }
  uint8_t thePeriod = rec[startIndex].length/31; // max is 255
  uint16_t min1Period = thePeriod*4/10;
  uint16_t max1Period = thePeriod*16/10;
  uint16_t min3Period = thePeriod*23/10;
  uint16_t max3Period = thePeriod*37/10;
  char command[13];
  command[0] = (char)((thePeriod - 110)/10 + 65);
  startIndex++; // begin with the first value
  uint8_t repeats = 2; // it is enough 2 times
  while(repeats)
  {
    char tmpCommand[13]; //= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    tmpCommand[0] = command[0];
    uint8_t currIndex = 1;
    for(uint16_t q = startIndex; q < startIndex+48; q += 4) // detect 1 sequence and write it to tmpCommand
    {
      uint8_t theTrit = 0;
      for(uint8_t ind = 0; ind < 4; ind++)
      {
        if((rec[q + ind].length >= min1Period) && (rec[q + ind].length <= max1Period)) bitClear(theTrit,ind);
        else if((rec[q + ind].length >= min3Period) && (rec[q + ind].length <= max3Period)) bitSet(theTrit,ind);
      }
      switch(theTrit)
      {
        case 0B1010: // LSB -> MSB
          tmpCommand[currIndex] = '0';
          break;
        case 0B0101:
          tmpCommand[currIndex] = '1';
          break;
        case 0B0110:
          tmpCommand[currIndex] = 'F';
          break;
      }
      currIndex++;
    }
    startIndex += 50;
    // if seq detected for the first time then write it from tmpCommand to command and don't compare
    if(repeats > 0)
    {
      for(uint8_t j=1; j<13; j++) command[j] = tmpCommand[j];
      repeats--;
    }
    else
    {
      if(command == tmpCommand) repeats--;
      else break;
    }
  }
  
  if(repeats)
  {
    Serial.println("NO sequence detected");
    return;
  }
  else
  {
    Serial.print("Command detected: ");
    for(uint8_t j=0; j<13; j++) Serial.print(command[j]);
    Serial.println();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - 

void findCorelation1(uint16_t numOfElements)
{
  uint32_t currTime = micros();
  uint16_t longestSequence = 1;
  uint16_t beginingIndex = 0;
  uint16_t differenceStep = 0;
  for(uint16_t i=0; i<numOfElements-4; i++)
  {
    for(uint16_t j=i+4; j<numOfElements; j++)
    {
      if(compare(i, j))
      {
        uint16_t startIndex = i;
        for(uint16_t k=i+1; k<numOfElements-3; k++)
        {
          if(compare(k, k+j-i))
          {
            if(k - startIndex >= longestSequence)
            {
              longestSequence = k - startIndex + 1;
              beginingIndex = startIndex;
              differenceStep = j;
            }
          }
          else break;
        }
      }
    }
  }
  Serial.print("Longest Sequence: ");
  Serial.println(longestSequence);

  uint16_t maxLength = 0;
  uint16_t maxLengthIndex = 0;
  uint8_t periodLength = 0;
  // find the biggest maximum
  for(uint16_t i=beginingIndex; i<beginingIndex+longestSequence; i++)
    if(maxLength < rec[i].length)
    {
      maxLength = rec[i].length;
      maxLengthIndex = i;
    }
  for(uint16_t i=beginingIndex; i<beginingIndex+longestSequence; i++)
  {
    if(maxLengthIndex < i) periodLength = i - maxLengthIndex;
    else
    {
      periodLength = maxLengthIndex - i;
      maxLengthIndex = i;
      break;
    }
  }
    Serial.println(maxLengthIndex);
    Serial.println(periodLength);
    Serial.println("Wait 4 sec..");
    delay(4000);
    
  for(uint16_t t=0; t<8; t++)
  {
    pinMode(RFPin, OUTPUT);
    for(uint16_t i=maxLengthIndex; i<maxLengthIndex+periodLength; i++)
    {
      digitalWrite(RFPin, rec[i].state);
      delayMicroseconds(rec[i].length);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - 

boolean compare(uint16_t a, uint16_t b)
{
  uint8_t delta = 13; // 4% derivation
  if(rec[a].state == rec[b].state)
  {
    if(rec[a].length > rec[b].length)
    {
      if(rec[a].length - rec[b].length < delta) return true;
      else return false;
    }
    else
    {
      if(rec[b].length - rec[a].length < delta) return true;
      else return false;
    }
  }
  else return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - 

void pinScan(uint8_t thePin)
{
  uint16_t index = 0;
  uint8_t currState = digitalRead(thePin);
  uint32_t currTime = micros();
  uint32_t startTime = millis();
  while(index < maxElements)
  {
    if(digitalRead(thePin) != currState)
    {
      rec[index].length = micros() - currTime;
      rec[index].state = currState;
      currTime = micros();
      currState = digitalRead(thePin);
      index++;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - 
