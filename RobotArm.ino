//#include <Servo.h>
#include <SPI.h>
#include <SD.h>
#include <VarSpeedServo.h>

File myFile;

const int chipSelect = 53;
const int continuousDelay=250;
const int servoSpeed=50;

VarSpeedServo myServo[6];
byte degreesLog[30][6]; // Contains the recorded servo degrees
            // recording max 30 point
byte logCount=0;
byte currentDegrees[6]{90,90,90,90,90,90};

byte degreesToRun[30][6];// Contains degrees to run
byte runCount=0;

String lastRunRecord="";
String tempText="";

void setup() 
{
  Serial.begin(9600);

  // Servos init
  // Servos pin 6,7,8,9,10
  // Pin 11 for continuous servo
  for(int i=0; i<6; i++){
    myServo[i].attach(i+6);
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("SD card OK.");

  myServo[5].write(90);
  delay(100);
  
  Serial.println("READY!");
}

void loop() 
{
  if (Serial.available())
  {

    // Read first parameter until ','
    String command = Serial.readStringUntil(',');
    Serial.println(command);

    // W, write degree in indexed servo
    // Coming data like "W,2,59"
    if(command=="W"){
      byte index=(Serial.readStringUntil(',')).toInt();
      byte degree=(Serial.readStringUntil(',')).toInt();
      if(degree>180)
          return;
      myServo[index].write(degree,servoSpeed,false);
      currentDegrees[index]=degree;
    }

    // WC, write for continuous servo
    // Coming data like "WC,dir"
    else if (command=="WC"){
      byte dir=(Serial.readStringUntil(',')).toInt();

      if(dir==0){
          myServo[5].write(0,255,false);
          currentDegrees[5]+=1;
      }

      else if(dir==1){
          myServo[5].write(180,255,false);
          currentDegrees[5]-=1;
      }

      delay(continuousDelay);
      myServo[5].write(90,255,false);
      }

    // WS, write for shoulder
    // Coming data like "WS,degree"
    // I use two servo for shoulder so, 
    // if writing a degree to left servo ,
    // write 180-degree another servo
    else if (command=="WS"){
        byte degree=(Serial.readStringUntil(',')).toInt();
        Serial.println(degree);
        myServo[3].write(degree);
        myServo[4].write(180-degree);
        currentDegrees[3]=degree;
        currentDegrees[4]=degree;
    }  

    // SD, save current degrees
    // Coming data like "SD"
    else if(command=="SD"){
      memcpy(degreesLog[logCount],currentDegrees,6);
      logCount++;
      currentDegrees[5]=90;
      tempText=logCount;
      tempText+=". Point saved!";
      Serial.println(tempText);
    }

    // SR, save record in sd card
    // Coming data like "SR,name"
    else if (command=="SR"){
      String name=Serial.readStringUntil(',');
      name+=".txt";

    if (!SD.begin(chipSelect)) {
      return;
    }

    if (SD.exists(name)){
      SD.remove(name);
    }

    myFile = SD.open(name, FILE_WRITE);
    myFile.close();
    myFile = SD.open(name, FILE_WRITE);

    if(myFile){
        WriteDegrees(myFile);
        myFile.close();
        tempText=name;
        tempText+=" saved!";
        Serial.println(tempText);
        currentDegrees[5]=90;
    }

    else{
        Serial.println("File can't open!");
    }  
    }

    // RR, run selected record.
    // Coming data like "RR,name"
    else if (command=="RR"){

        String name=Serial.readStringUntil(',');
        
        name+=".txt";
        Serial.println(name);
        if(SD.exists(name))
        {
            myFile = SD.open(name);
            if(myFile)
            {
              ReadDegrees(myFile);
            }
            else{
              Serial.println("File can't open");    
            }            
            myFile.close();
        }
        else{
            tempText=name;
            tempText+=" not exists!";
            Serial.println(tempText);
        }
        while(1){
        Playback();
        }
    }
    // H, go home.
    // Coming data like "H"
    else if (command=="H"){
        Serial.println("Homing..");
        for(int i=0; i<6; i++){
            myServo[i].write(90,servoSpeed,true);
        }
        logCount=0;
        runCount=0;
        currentDegrees[5]=90;
        Serial.println("Homing Done!");
    }
    // GR, get record list int sd card.
    // Coming data like "GR"
    else if (command=="GR"){

        File root = SD.open("/");
        if(root)
        {
          SendRecordName(root);
          root.close();
          Serial.println("OK");
        }
        else{
          Serial.println("Root can't open!");
        }
    }
  }
  delay(10);
}

// Writing recorded servos' degrees in text file
void WriteDegrees(File myFile)
{
  for(int i=0; i<logCount; i++){
    for(int j=0; j<6; j++){
           myFile.println(degreesLog[i][j]);
       }   
  }
  logCount=0;
}

// Reading saved degrees in text file and assignment to degrees to run
void ReadDegrees(File myFile)
{
    int y=0;
    runCount=0;
    // Read from the file until there's nothing else in it:
    while(myFile.available()){
        int degree=myFile.parseInt();
        //Serial.print("DEGREE:");
        //Serial.println(degree);
        degreesToRun[runCount][y]=degree;
        y++;
        if(y==6){
          y=0;
          runCount++;  
        }
    }
}
// Play selected record
void Playback(){
    Serial.println("Playback..!");
    for(int i=0; i<runCount; i++){
        for(int j=0; j<6; j++){
            //Serial.println(degreesToRun[i][j]);
            if(j==5){
                PlayContinuous(degreesToRun[i][j]);
            }
            else if (j==3 ||j==4){
                myServo[3].write(degreesToRun[i][j],servoSpeed,false);
                myServo[4].write(180-degreesToRun[i][j],servoSpeed,true);
                delay(50);
            }
            else{
                myServo[j].write(degreesToRun[i][j],servoSpeed,true);
            }
            delay(50);
        }
    }
    Serial.println("Playback done!");
}
void PlayContinuous(int value){
    if(value<90) {
      myServo[5].write(180,255,false);
      delay((90-value)*continuousDelay);
    }
    
    else if(value>90){
      myServo[5].write(0,255,false);
      delay((value-90)*continuousDelay);
    }
    myServo[5].write(90,255,false);
}

// Sends record name to selectable list
void SendRecordName(File dir) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
     break;
  }
  Serial.println(entry.name());
  delay(200);
}
}
