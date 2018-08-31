/*
 * Project:Intel conveyor belt
 * Manager: Logesh J
 * Architect: Raghul
 * Edited by: Amith J
 * Edited on : 22 Aug 2018
 * The motor is configured at 3200 steps for one full rotation i.e 360 digrees. 1 step=0.1125 digree 
 * In this code motor covers 56.25 digrees for 500 steps
 * motor direction Direction: when pin 5 is LOW then motor rotates in CWC(counter clock wise direction) and whemn it is HIGH motor rotates in CW(clock wise ) direction.
 * 
*/
#include<Wire.h>    // Library for I2c
#include<QueueArray.h>    //Lib for handling Queues
#include<EEPROM.h>        //Library for EEPROM
/*---------Global variable declarations----------------------*/
int timer1,timer2,sensor_1=3,sensor_2=2,pos_addr=1,sp_addr=2;
byte Position=0;//Changed
unsigned int count=0,Time1,Time2;
long debouncing_time =50; //Debouncing Time in Milliseconds
volatile unsigned long last_micros;
unsigned int divert=1,straight=0,count1=0;
byte pos=0,c=0;
volatile int Status=10;

QueueArray <byte> queue;
/*-----------------Global variable declarations ends here---------------------*/

void writer()   //Sending the request for speed update at the beginning of the code
{
  Wire.beginTransmission(8); // transmit to device #8 address of the slave device
  //Wire.write("x is ");        // sends five bytes
  Wire.write(5);              // sends one byte, 5 indicates opcode for the speed
  Wire.endTransmission();    // stop transmitting

  //x++;
  delay(500);
}

void decision()   //Sending the request for decission
{
  Wire.beginTransmission(8); // transmit to device #8
  //Wire.write("x is ");        // sends five bytes
  Wire.write(6);              // sends one byte, 6 indicates data for divert operation
  Wire.endTransmission();    // stop transmitting

  //x++;
 // delay(2000);
}
void reader()     //after request reading the speed 
{
  Wire.requestFrom(8,1);    // request 1 bytes from slave device #8

  while (Wire.available()) { 
    byte c = Wire.read(); // receive a byte as character
    EEPROM.write(sp_addr,c);    // Writing the received data to EEPROM at address 2nd byte,
    Serial.print(c);         // print the character
  }

  delay(500);
}

void setup() {
    
  pinMode(sensor_2,INPUT_PULLUP);   // Initilizing the sensor pin as input
  pinMode(7,INPUT_PULLUP);          //Enable pin 
  pinMode(5,OUTPUT);                // Stepper motor Direction pin 
  pinMode(6,OUTPUT);                // clock pulse to the Stepper motor
  pinMode(8,OUTPUT);                // Home position sensor port maping
  Serial.begin(115200);              //Initilizing the UART with 115200 baud rate
  Wire.begin();                       // initilizing the I2C function
  Serial.println("requesting");       
  /*Requesting the speed from Slave (sheild device)*/
  writer();                             
  delay(1000);
   Serial.println("reading");
  reader();           //Reading the speed
 delay(5);
// EEPROM.write(pos_addr,0)

  homepos();    // calling the homeposition sensor function, in this Arm will starts from diverting position
 attachInterrupt(digitalPinToInterrupt(sensor_2), Sensor2, FALLING);//Attaching the interrupt for sensor 2 which calls motor run function

}
/*-------------Main Function starts here---------------------------------*/
void loop() {

if(Status==20)  //When interrupt comes(object arrives) then Status value sets to 20(integer value)
{
  Serial.println("requesting");
  decision() ;      // requesting position to move, Divert position=0, straight position=1
   delay(5);
    
 
    Wire.requestFrom(8, 1);    // read 1 byte from slave device #8
    
  while (Wire.available()) { 
     c = Wire.read();
     
  }
    if(c==255)  //if There is no data (0 or 1) then controller sends CMD to Slave to call conveyor stop function
    {
      Wire.beginTransmission(8); 
  //Wire.write("x is ");       
  Wire.write(7);              
  Wire.endTransmission();    
    }
      queue.enqueue(c);
    
    
    Serial.println(c,BIN); 
    
 
  Serial.println("calling motor func");
  /*------------Reads speed opcode and makes decission to call Three different speed*/
  /*Low SPEED @12.5HZ(conveyor speed setup) = 2
   * MID SPEED @25HZ(conveyor speed setup) =3
   *  Full SPEED @50HZ(conveyor speed setup) =4
   */
    if(EEPROM.read(sp_addr)==2)
  {
    motorL();       //Motor Run time=1200milli secs
  }
  if(EEPROM.read(sp_addr)==3)
  {
  motorM();         //Motor Run time=800 milli secs
  }
 
 if(EEPROM.read(sp_addr)==4)
  {
    motorF();         //Motor Run time=400 milli secs
  }
  Status=0;    //Clearing Status value
}

}
/*-----------------------Main Function ends here-------------------*/

/*------------ISR function for Sensor_2 Interrupt-------------------*/
void Sensor2()
{ 
  if((long)(micros() - last_micros) >= debouncing_time * 10000) //Debounce functions to reduce Jitters
    {  Serial.println("interrupted");
               Status=20;     //Sets Status value to 20
           last_micros = micros();
          
    }
}
/*------------------------Home position function -----------------------------*/
//this function The diverting Arm will come towards the home position sensor and starts from beginning
void homepos()
{ int steps;
  digitalWrite(5,LOW);//if 5th pin is low then Motor moves in CWC 
  while((digitalRead(7)==LOW))
  {
    digitalWrite(6,HIGH);
    delay(3);               //50% duty cycle on time=3millisecs, off time= 3 millisecs
    digitalWrite(6,LOW);
    delay(3);
    steps++;
  }
  steps=0;
  return;
}

void motorF()   //Function for fast speed
{
      //cli();
     Position=EEPROM.read(pos_addr); //reading position from EEPROM
    Serial.println("in motor funcF");
    Serial.print("Position before motor actuation :");
    Serial.println(Position);
    pos=queue.dequeue();
       
    if(pos==0)        // Same position it will not move any where
    {
      if(Position==0)
      {
        Serial.println("returning same position : 0");
        return;
      }
      else
      {                     //Else diverter Arm will move to Straight position 1
        
         Serial.println("Moving to position : ");
         Serial.println(pos);   
         EEPROM.write(pos_addr,pos);    //storing the present position to EEPROM for feedback
               
         digitalWrite(5,LOW);

         count=0;
                  
          while(count<20)       //Stepper motor Ramp-up function to energiese the motor coils
          {
            digitalWrite(6,HIGH);
            delayMicroseconds(1000);
            digitalWrite(6,LOW);
            delayMicroseconds(1000);
            count++;
          }
          count=0;
          while(count<375)        // accelerating at high speed
          {
            digitalWrite(6,HIGH);
            delayMicroseconds(200);   
            digitalWrite(6,LOW);
            delayMicroseconds(200);
            count++;
          }
              count=0;
                 while(count<85)  //Deaccelerating function for smooth movement(stop) or Applying break slowly
              {
                digitalWrite(6,HIGH);
                delayMicroseconds(1000);
                digitalWrite(6,LOW);
                delayMicroseconds(1000);
                count++;
              }
              count=0;
            
            
              while(count<20)
              {
                digitalWrite(6,HIGH);
                delayMicroseconds(1000);
                digitalWrite(6,LOW);
                delayMicroseconds(1000);
                count++;
              }
              count=0; 
        
              //Position=1;
              return;            
      }
    }    
    if(pos==1)
    {
      if(Position==1)         //This function is same as above logic. please refere above for rest of the functions i.e Medium speed and full speed.
      {
        Serial.println("returning same position : 1");
        return;
      }
      else
      {
        
        //go in straight direction
           
        int i;
        Serial.print("Moving to position : ");
        Serial.println(pos);
        EEPROM.write(pos_addr,pos);
        
        digitalWrite(5,HIGH);

         count=0;
                  
          while(count<20)
          {
            digitalWrite(6,HIGH);
            delayMicroseconds(1000);
            digitalWrite(6,LOW);
            delayMicroseconds(1000);
            count++;
          }
          count=0;
          while(count<375)
          {
            digitalWrite(6,HIGH);
            delayMicroseconds(200);
            digitalWrite(6,LOW);
            delayMicroseconds(200);
            count++;
          }
              count=0;
                 while(count<85)
              {
                digitalWrite(6,HIGH);
                delayMicroseconds(1000);
                digitalWrite(6,LOW);
                delayMicroseconds(1000);
                count++;
              }
              count=0;
            
            
              while(count<20)
              {
                digitalWrite(6,HIGH);
                delayMicroseconds(1000);
                digitalWrite(6,LOW);
                delayMicroseconds(1000);
                count++;
              }
              count=0;        
      }      
    }
}

void motorM() //Function for mid speed 
{
      //cli();
      Position=EEPROM.read(pos_addr);
    Serial.println("in motor funcM");
    Serial.print("Position before motor actuation :");
    Serial.println(Position);
    pos=queue.dequeue();    
    if(pos==0)
    {
      if(Position==0)
      {
        Serial.println("returning same position : 0");
        return;
      }
      else
      {
        
         Serial.println("Moving to position : ");
         Serial.println(pos);   
         //Position=pos;
         EEPROM.write(pos_addr,pos);      
         digitalWrite(5,LOW);

         count=0;
                  
          while(count<20)
          {
            digitalWrite(6,HIGH);
            delayMicroseconds(1000);
            digitalWrite(6,LOW);
            delayMicroseconds(1000);
            count++;
          }
          count=0;
          while(count<375)
          {
            digitalWrite(6,HIGH);
            delayMicroseconds(735);
            digitalWrite(6,LOW);
            delayMicroseconds(735);
            count++;
          }
              count=0;
                 while(count<85)
              {
                digitalWrite(6,HIGH);
                delayMicroseconds(1000);
                digitalWrite(6,LOW);
                delayMicroseconds(1000);
                count++;
              }
              count=0;
           
            
              while(count<20)
              {
                digitalWrite(6,HIGH);
                delayMicroseconds(1000);
                digitalWrite(6,LOW);
                delayMicroseconds(1000);
                count++;
              }
              count=0; 
              return;            
      }
    }    
    if(pos==1)
    {
      if(Position==1)
      {
        Serial.println("returning same position : 1");
        return;
      }
      else
      {
        
        
           
        int i;
        Serial.print("Moving to position : ");
        Serial.println(pos);
       // Position = pos;
        EEPROM.write(pos_addr,pos);   
        digitalWrite(5,HIGH);

         count=0;
                  
          while(count<20)
          {
            digitalWrite(6,HIGH);
            delayMicroseconds(1000);
            digitalWrite(6,LOW);
            delayMicroseconds(1000);
            count++;
          }
          count=0;
          while(count<375)
          {
            digitalWrite(6,HIGH);
            delayMicroseconds(735);
            digitalWrite(6,LOW);
            delayMicroseconds(735);
            count++;
          }
              count=0;
                 while(count<85)
              {
                digitalWrite(6,HIGH);
                delayMicroseconds(1000);
                digitalWrite(6,LOW);
                delayMicroseconds(1000);
                count++;
              }
              count=0;
       
            
              while(count<20)
              {
                digitalWrite(6,HIGH);
                delayMicroseconds(1000);
                digitalWrite(6,LOW);
                delayMicroseconds(1000);
                count++;
              }
              count=0;             
      }      
    }
}

void motorL() //Function for LOW speed 
{
      
      Position=EEPROM.read(pos_addr);
    Serial.println("in motor funcL");
    Serial.print("Position before motor actuation :");
    Serial.println(Position);
    pos=queue.dequeue(); 
    //pos=0;   
    if(pos==0)
    {
      if(Position==0)
      {
        Serial.println("returning same position : 0");
        return;
      }
      else
      {
        
         Serial.println("Moving to position : ");
         Serial.println(pos);   
         //Position=pos;
         EEPROM.write(pos_addr,pos);      
         digitalWrite(5,LOW);

         count=0;
                  
          while(count<20)
          {
            digitalWrite(6,HIGH);
            delayMicroseconds(1000);
            digitalWrite(6,LOW);
            delayMicroseconds(1000);
            count++;
          }
          count=0;
          while(count<375)
          {
            digitalWrite(6,HIGH);
            delayMicroseconds(1267);
            digitalWrite(6,LOW);
            delayMicroseconds(1267);
            count++;
          }
              count=0;
                 while(count<85)
              {
                digitalWrite(6,HIGH);
                delayMicroseconds(1000);
                digitalWrite(6,LOW);
                delayMicroseconds(1000);
                count++;
              }
              count=0;
           
            
              while(count<20)
              {
                digitalWrite(6,HIGH);
                delayMicroseconds(1000);
                digitalWrite(6,LOW);
                delayMicroseconds(1000);
                count++;
              }
              count=0; 
              return;            
      }
    }    
    if(pos==1)
    {
      if(Position==1)
      {
        Serial.println("returning same position : 1");
        return;
      }
      else
      {
        
        //go in straight direction
           
        int i;
        Serial.print("Moving to position : ");
        Serial.println(pos);
       // Position = pos;
        EEPROM.write(pos_addr,pos);   
        digitalWrite(5,HIGH);

         count=0;
                  
          while(count<20)
          {
            digitalWrite(6,HIGH);
            delayMicroseconds(1000);
            digitalWrite(6,LOW);
            delayMicroseconds(1000);
            count++;
          }
          count=0;
          while(count<375)
          {
            digitalWrite(6,HIGH);
            delayMicroseconds(1267);
            digitalWrite(6,LOW);
            delayMicroseconds(1267);
            count++;
          }
              count=0;
                 while(count<85)
              {
                digitalWrite(6,HIGH);
                delayMicroseconds(1000);
                digitalWrite(6,LOW);
                delayMicroseconds(1000);
                count++;
              }
              count=0;
       
            
              while(count<20)
              {
                digitalWrite(6,HIGH);
                delayMicroseconds(2000);
                digitalWrite(6,LOW);
                delayMicroseconds(2000);
                count++;
              }
              count=0;             
      }      
    }
}


