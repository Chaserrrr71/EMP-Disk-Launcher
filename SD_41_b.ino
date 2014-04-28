/*
In this version, I have Re-written all of the code from the ground up to be better optimized and user friendly.
Added reading voltages for done functionality.
Added previous function of being able to discharge while charging.
Added redundancy to the "done" command. 
Rewrote the done func to include the ASK functionality. ver 34
Added state synchronization. ver 35
No change ver 36
No change ver 37
Finalized state synchronization. ver 38
Added manual discharge detection & "always stop charging" ver 39
Removed Digital potentiometer code from file. ver 40



Code Written by Chase Mendenhall
 Written for our Capstone Senior Design Project. Goal: Wirelessly control 
 an EMP Disk Launcher. Allow the user to select their power level. Have 
 built in double checks to dis-allow misfires or false fires.
 
 
 PINOUT:
 

 Relays:
 charge button relay : pin 5
 stop charge button relay : pin 6
 discharge and safety relay : pin 7
 Fire relay : pin 8
 
 Xbee:
 Xbee pin 1 - 3.3v arduino
 xbee pin 2 - pin 0 arduino
 Xbee pin 3 - pin 1 Arduino
 Xbee pin 10 - Arduino GND
 
 Voltage Measurement:
 Pin A0
 
 Manual Mode pin A1
*/
//Define commands
  #define mess_head 254       //message header
  #define fire_comm 177         //fire command
  #define discharge_comm 103    //dichage command
  #define confirm_comm 231      //confirm command
  #define charge_comm 192       //charge command
  #define done_comm 134         //done charging command
  #define notdone_comm 155      //not done charging command
  #define askfordone_comm 129   //remote asking for charging state
  #define askforcharge_comm 108 //remote asking for the current charge on the system
  
//Define Outputs
  #define StartCharging 5  //connect to the relay that is across the "start charging" button on the powersupply
  #define StopCharging 6  //connect to the relay that is across the "stop charging" button
  #define SafetyRelay 7   //connect to the relay controls discharging through thte resistor
  #define FireRelay 8     //connect to the relay that fires the system 
  
  #define SendDelay 150 //to try to fix the delay in communication
  
  #define measureCap A0 //pin to read voltage across caps
  #define ManualMode 12 //pin to dissengage the arduino.
  
  #define Cap_Calibration 10//Use to adjust sensitivty when measuring voltage difference on caps.
  #define time_Calibration 750 //Use to adjust how frequently to read the compare the voltage on the caps.
//include headers

  #include<power.h>
  
//declare vars
byte state = 1; //for fsm
byte firstpass; // falg to execute code once.
struct myMessage{unsigned char head,comm,val,currentstate,CS;}; //defining struct's structure
byte value = 200; //to hold a value
unsigned long int timeout; //to hold activity for timeout
boolean charged = false; // whether or not the system has been charged.

void setup() {
  //set up pins
  pinMode(StartCharging,OUTPUT);
  pinMode(StopCharging,OUTPUT);
  pinMode(SafetyRelay,OUTPUT);
  pinMode(FireRelay,OUTPUT);
  pinMode(ManualMode,INPUT_PULLUP);   //manual mode input pin
  Serial.begin(9600);
}

void loop() {
 
  
  switch(state){//state of charging system, 1 waiting for charge command
  
   case 1:
    while(digitalRead(ManualMode) == LOW){
         //doNOTHING
         delay(250); //since we are doing nothing we may as well slow everything down
       }
    if(firstpass == 1){ //code to happen once 
        firstpass = 0; // set flag
        timeout = millis();
    }
       
       value = ReceiveandConfirm(2000); //recieve charge command and store the value received.
       if (value <= 100 && value > 0){ //check to see if that value was valid
         timeout = millis(); //take time for activity
         state = 2; //move to next state
         firstpass = 1; //set flag
        }
        //check timeout, dicharge if needed
       if(millis() - timeout >= 20000 && charged == true){
        SafetyDischargeFunc(); //discharge caps
       }
     break;
     
    case 2:
      if(firstpass == 1){ //code to happen once 
          firstpass = 0; // set flag
          timeout = millis();
      }
    
     if(doneFunc(ChargeFunc(value)) == true){ //wait for the caps to finish charging. If no error, move to next state
        state = 3; //move along to next state
        firstpass = 1; //set flag
     }    
     else{//something went wrong.. or a discharge button was pressed.
       SafetyDischargeFunc();//discharge 
       state = 1; //if not, discharge and return to beginning
     }
     break;
     
    case(3):
     if(firstpass == 1){ //code to happen once 
        firstpass = 0; // set flag
        timeout = millis(); //take activity time
    }
     if(digitalRead(ManualMode) == LOW){//check for do nothing switch
        //need to do nothing as fast as possible
        state = 1;
        firstpass = 1;
       break; 
     }
     value = ReceiveandConfirm(2000); //hold the command in the message
     
     if (value == fire_comm){//check if the message was fire command
       FireInTheHole(); //fire the projectile
       value = 200;//reset value
       state = 1; //return to beginning
     }
     else if(value == discharge_comm){//check to see if command was discharge command
       SafetyDischargeFunc(); //discharge caps
       value = 200;  //reset value
       state = 1;   //return to beginning
     }
     else if (value == askforcharge_comm){//remote is asking for the current level on the caps.
       SendandConfirm(notdone_comm, map(analogRead(measureCap),0,1024,1,100),200); //send the current value on the caps  
     }
     //check for timeout
     if(millis() - timeout >= 20000 && charged == true){
        SafetyDischargeFunc(); //discharge caps
        state = 1;
      }
     break;
  }
}

//function to start charging the capacitors
int ChargeFunc(int sensorval){
   charged = true; //update charged flag
   sensorval = map(sensorval,1,100,1,1000);//map sensor value to potentiometer 0 - 94k
 
   digitalWrite(SafetyRelay,HIGH);//open discharge relay
   digitalWrite(StartCharging,HIGH);//"press" start charge button
   delay(100); //wait a sec
   digitalWrite(StartCharging,LOW);//release start charge button
   return sensorval;
}

//function to safely dischrge the caps through the resistor
void SafetyDischargeFunc(){
   digitalWrite(StopCharging,HIGH);//"press stop charging button
   delay(100); //wait a sec
   digitalWrite(StopCharging,LOW);//release stop charging button
   digitalWrite(SafetyRelay,LOW);//close discharge relay
   charged = false; //set flag caps are no longer charged
}

//function to fire the caps and secure the device
void FireInTheHole(){
   digitalWrite(FireRelay,HIGH);//close the fire relay
   delay(1000);// wait a sec
   digitalWrite(FireRelay,LOW);//open the fire relay
   delay(500); //delay to ensure the relay doesnt get stuck
   digitalWrite(SafetyRelay,LOW);//close the discharge relay
}

//function to read voltage on caps to determine whether charging has stopped, also handles sending done command.
int doneFunc(int chargeto){
  // measure voltage on cap for change.. done charging code here.
      boolean finished = false; //falg for done charging
      boolean error = true; //flag for error in transmission
      unsigned long int currentTime; //take current time for a little time out loop

      int offset = 0; //place to store a number
      while(millis() - timeout <= 60000){
        
        //need to poll for discharge, safety first folks!
        value = ReceiveandConfirm(2000); //receive command and store the value received.
        if(value == discharge_comm){//check to see if command was discharge command
              value = 200; //reset value
              return false;
        }
        
        //didnt discharge, lets see if the remote asked for the current state
        if(value == askfordone_comm){ //it asked for the state
          if(finished == true){ //the caps have stabalized send the done command
            if(SendandConfirm(done_comm,0,2000) ==true){ //done command confirmed exit function
              return true;
            }
          }
          else{ // not finished, send current state.
            SendandConfirm(notdone_comm, map(analogRead(measureCap),0,1024,1,100),200); //send the current value (used to update led string)
          }
        }
        
       //read caps every time_Calibration msec
          if(millis()- timeout - offset >= time_Calibration && finished == false){             
             offset = offset + time_Calibration; //sure that the if statement isn't always true.
             if(chargeto - analogRead(measureCap) < 0){ //compare capvoltage to current voltage if <Cap_Calibration difference charging is done
                 digitalWrite(StopCharging,HIGH);//press stop charging button
                 delay(100); //wait a sec
                 digitalWrite(StopCharging,LOW);//release stop charging button
                 finished = true; //set flag to exit loop
             }
           }
           
           //lets check to see if the maual switch flipped.
           if(digitalRead(ManualMode) == LOW){//check for do nothing switch
              //need to do nothing as fast as possible
              state = 1;
              firstpass = 1;
              break; 
           }
      }
      
      //if we made it here we timed out after 60 sec loop. 
      return false;    
}

// function to read incoming messages and return them Protocol: (header)(command)(value)(checksum)
int readCommand(struct myMessage &message, int timeout){
	
	long breaktime = millis() + timeout;
	
	//set Arduino serial library to passed in timeout to make sure Serial calls don't block for longer than expected
	Serial.setTimeout(timeout);
	
	//temp array to parse the message
	byte temp[5];
	//a place to calculate checksum
	byte CS;
	
	//we'll loop while we have the time
	while( millis() < breaktime) {
		
		//look for header. This covers if the buffer is empty, as it would return -1 which isn't 254
		if(Serial.peek() == 254){
			
			//we can treat a struct as a binary block of data, and copy out a whole block of data at once
			//this will also automatically block while the whole message comes in. 
			//reference says we should use byte type, but this function requires a char cast for some reason
			Serial.readBytes((char*)temp, 5);
			
			//now run a calculate checksum
			for (int i =0; i<=3; i++)
				CS ^= temp[i];
			
			//compare checksums
			if (CS == temp[4]) {
				//checksum good, copy into message struct
				
				message.head = temp[0];
				message.comm = temp[1];
				message.val = temp[2];
                                message.currentstate = temp[3];
				message.CS = temp[4];
				return true;
			}	
			else //checksum no good, message trashed
				return false;
		}
		//if we didn't find the header, trash the current byte if any by reading it
		else
			Serial.read();
	}
	
	//if we got here, time expired and we never got a message
	return false;
}

//Receives and confirms messages, only reads messages if they are both available and match header and CS... Kind of messy, may need refining.	
int ReceiveandConfirm(byte timeout){
  struct myMessage currentMessage; //struct to hold the current message
  struct myMessage previousMessage; //struct to transfer new to old for holding
  if(Serial.available()){
          if(readCommand(currentMessage,timeout) == true){//if recieve successfully
              sendCommand(currentMessage.comm,currentMessage.val); //send the same message back and check for confirm
              previousMessage = currentMessage; //holds the command to to determine charge or discharge
              delay(SendDelay); 
                  if (readCommand(currentMessage,timeout) == true){
                     if(currentMessage.comm == confirm_comm){ //after message is confirmed, check to see if is discharge or fire
                       if(previousMessage.comm == fire_comm){ //checks to see if the message contained fire
                       return fire_comm; // used to return the findings of receiveandconfirm
                       }
                       else if(previousMessage.comm == discharge_comm){ //checks to see if the message contained discharge
                       return discharge_comm; // used to return the findings of receiveandconfirm
                       }
                       else if(previousMessage.comm == charge_comm){//checks to see if the message contained charge
                         return previousMessage.val; //returns the value to charge to
                       }
                       else if(previousMessage.comm == askfordone_comm){//checks to see if the message contained askfordone comm
                         return askfordone_comm; //returns the command
                       }
                       else if(previousMessage.comm == askforcharge_comm){//checks to see if the message contained askforcharge comm
                         return askforcharge_comm; //returns the command
                       }
                     }
                     else{
                      return 200; //message wasn't confirmed 
                     }
                  }
                  else{
                   return 200; //timeout or invalid message
                  }
           }
           else{
              return 200; //timeout or invalid message
           }
  }
  else {
    return false; //no data ready
  }
}


//Send signals via xbee. messages are as followed (header)(command byte)(value)(checksum)
void sendCommand(unsigned char command, unsigned char value) {
  
  byte CS = 254 ^ command ^ value ^ state; //checksum byte
  
  Serial.write(mess_head); //send header
  Serial.write(command); //send command
  Serial.write(value);   //send value
  Serial.write(state);  //send state
  Serial.write(CS);   //send checksum
  
}


int SendandConfirm( byte command, byte value, long timeout){
  struct myMessage currentMessage; //struct to hold the current message
  sendCommand(command,value); //sends command
  delay(SendDelay); 
  if(readCommand(currentMessage, timeout) == true){//gets new message and checks CS and header
    if(currentMessage.comm == command && currentMessage.val == value){ // checks command and value
          sendCommand(confirm_comm,0); //sends confirm message
          delay(SendDelay);
          return true;// if message was confirmed
        } 
  } else{
    return false;//if message wasn't confirmed
  }
  return false; //to confirm that the message wasn't confirmed
}


