/*In this version I have overhauled all of the code. 
The purpose is to streamline and optimize the previous version using better programming techniques.
The functionality remains the same.
Added new LED functions (Lead user through program with LEDS)
Added previous function of being able to discharge while charging.
Rewrote the done func to include the ASK functionality. ver 34
Added state synchronization. ver 35
Added Visual indicator for states syncing ver 36
Squashed bugs that arrize when the communication has an error ver 37
Finalized state synchronization. ver 38



Code Written by Chase Mendenhall
 Written for our Capstone Senior Design Project. Goal: Wirelessly control 
 an EMP Disk Launcher. Allow the user to select their power level. Have 
 built in double checks to dis-allow misfires or false fires.
 
PIN SETUP
 Charging Button:
 One side  - GND
 The other - pin 3
 
 Discharge Button:
 One side  - GND
 The other - pin 2
 
 Fire Button: 
 One side  - GND
 The other - pin A2, Resistor to GND
 
 Potentiometer:
 One side  - +3.3V
 Middle    - pin A3
 The other - GND
 
 Xbee:
 Xbee pin 1 - 3.3v arduino
 xbee pin 2 - pin 0 arduino
 Xbee pin 3 - pin 1 Arduino
 Xbee pin 10 - Arduino GND
 
 Chrlieplexed LEDs 6,7,8,9
 
 LED rings:
 charge button : pin 4
 discharge button : pin 5
 fire button : pin 10
 
 Power LED: pin 11
 */
 
 
  #include <avr/sleep.h> //sleep command header
 
 //charlieplexing defines
  #define PIN_CONFIG 0 //selects the first set of data in the matrix i.e. { OUTPUT, OUTPUT, INPUT, INPUT }
  #define PIN_STATE 1 //selects the second set of data in the matrix i.e.  { HIGH, LOW, LOW, LOW }
  #define LED_COUNT 11 //defines number of leds
  
 //pin mapping for charli array
  #define A 9  
  #define B 8
  #define C 7
  #define D 6
  
 //pin mappings for buttons and LEDs
  #define ChargeButton 3
  #define Disbutton 2
  #define FIREbutton A2
  #define charge_LED 4 //LED ring around charge button
  #define discharge_LED 5 //LED ring around discharge button
  #define fire_LED 10 //LED ring around fire button
  #define power_LED 11 //LED ring around fire button
  #define pot A3  //Potentiometer 

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
 
 #define SendDelay 150 //fixes the missing done signal.
 
 #define time_Calibration 250 //Use to adjust how frequently to read the compare the voltage on the caps.
 
 //Init global vars
 int matrix[LED_COUNT][2][4] = { //Charliplex matrix
// PIN_CONFIG PIN_STATE
// A B C D A B C D
{ { OUTPUT, OUTPUT, INPUT, INPUT }, { HIGH, LOW, LOW, LOW } }, // AB 0
{ { OUTPUT, OUTPUT, INPUT, INPUT }, { LOW, HIGH, LOW, LOW } }, // BA 1
{ { INPUT, OUTPUT, OUTPUT, INPUT }, { LOW, HIGH, LOW, LOW } }, // BC 2
{ { INPUT, OUTPUT, OUTPUT, INPUT }, { LOW, LOW, HIGH, LOW } }, // CB 3
{ { OUTPUT, INPUT, OUTPUT, INPUT }, { HIGH, LOW, LOW, LOW } }, // AC 4
{ { OUTPUT, INPUT, OUTPUT, INPUT }, { LOW, LOW, HIGH, LOW } }, // CA 5
{ { OUTPUT, INPUT, INPUT, OUTPUT }, { HIGH, LOW, LOW, LOW } }, // AD 6
{ { OUTPUT, INPUT, INPUT, OUTPUT }, { LOW, LOW, LOW, HIGH } }, // DA 7
{ { INPUT, OUTPUT, INPUT, OUTPUT }, { LOW, HIGH, LOW, LOW } }, // BD 8
{ { INPUT, OUTPUT, INPUT, OUTPUT }, { LOW, LOW, LOW, HIGH } }, // DB 9
{ { OUTPUT, OUTPUT, OUTPUT, OUTPUT }, { LOW, LOW, LOW, LOW } }, // OFF 10
};

byte state = 1; //controls the state of the remote. switch statement.
byte firstpass = 1; //first pass flag
byte sensorvalue; //value from the potentiometer
struct myMessage{unsigned char head,comm,val,currentstate,CS;}; //defining struct's structure
unsigned long int timeout = millis(); //var to hold millis() for timeout purposes.
byte temp_sensor;  //place to hold sensor value
int temp_val; //temporary place to store a variable necessary for decisions throughout the program.
int offset = 0; //place to store a number for timing later




void setup() {
  Serial.begin(9600);
  pinMode(ChargeButton,INPUT_PULLUP);   //Charge button
  pinMode(FIREbutton,INPUT_PULLUP);    //Discharge Button
  pinMode(Disbutton,INPUT_PULLUP);    //FIRE Button
  pinMode(charge_LED,OUTPUT);            //sets the led ports to output
  pinMode(discharge_LED,OUTPUT);            //sets the led ports to output
  pinMode(fire_LED,OUTPUT);            //sets the led ports to output
  pinMode(power_LED,OUTPUT);            //sets the led ports to output
  set_sleep_mode(SLEEP_MODE_PWR_SAVE); //setup low power mode "sleep" mode
  sleep_enable();                    //enable sleep so that it can be entered
  digitalWrite(power_LED,HIGH); //turns on the power LED never turns off
  
  
}



void loop() {

  
  switch (state){ //state of remote 1 = wating state, 2 = charging state, 3 = more user input (fire/discharge) 4 = firing state
    
    case 1: //reading buttons state, waiting for user input, also control charliplexxed LEDs
      if (firstpass == 1){ //code to happen once
        digitalWrite(charge_LED,HIGH); //turn on charge LED ring
        digitalWrite(discharge_LED,LOW); //turn off discharge LED
        digitalWrite(fire_LED,LOW); //turn off discharge LED
        firstpass = 0; // set flag back
        timeout = millis();
        temp_sensor = map(analogRead(pot),0,1023,0,100);
        
      }
      //read analog potentiometer and maps it from 0 - 100
      sensorvalue = map(analogRead(pot),0,1023,0,100);
            
            
          //scan power to charliplex leds
      for( int led_var = 10; led_var > 0;led_var--){     //led_var = which of the LED maticies to load 
        if(sensorvalue >= led_var*10-9){                 //i.e. let sensorval = 100 and ledvar starts at 10. 100 >= 91 turns on the highest LED
          turnOn(led_var-1);                             //led_var - 1 because entry 10 is used for turning all LEDs off.
        }
        else if (sensorvalue == 0){                      //else turn off all leds
          turnOn(10);
        }
      }
      
      //check charge button for activity
      if(digitalRead(ChargeButton) == LOW){
        if(sensorvalue == 0){ //checks to see if 0 was selected.
            state = 1;          //if so set state to 1 (waiting state). There is no need to send a 0 charge command.
            break;
          }
          temp_val = SendandConfirm(charge_comm,sensorvalue,2000);
          
          if( temp_val == false){ //send charge command and move on if successful
            firstpass = 1; //set flag so that timeout resets
            state = 1; //return to state 1
            //blink error led
            ErrorFunc();
            break;
          }
          else if(  temp_val == true){
           state = 2; //sets state to 2 (charging state) 
          }
        
      }
      
      //halfway to sleep, check for potentiometer update.
      if(millis() - timeout >= 10000){ 
        if(abs(temp_sensor - sensorvalue) > 1){ //if they are different by one run through firstpass again.
          firstpass = 1;
        }
      }
      
      //check timeout, go to sleep
      if(millis() - timeout >= 20000){
        enterSleep();//GO to sleep.
      }
      
    break;
    
    case 2: //charging state, send charge command, double check it, wait for done command. This stage is completely automated, there shouldn't be any user input.
          timeout = millis();  
      
          if (doneFunc() == false){ //wait for done command, with style
            firstpass = 1; //set flag so that timeout resets
            state = 1; //return to state 1
          }
          else{// done must have been true
          firstpass = 1; //set flag
          state = 3;// next state
          }
    
    //end case 2
    break;
    
    
    case 3:
      if(firstpass == 1){ //code to run once
        firstpass = 0; //reset flag.
        timeout = millis(); // set up timeout
        digitalWrite(charge_LED, LOW);
        digitalWrite(discharge_LED,HIGH);
        digitalWrite(fire_LED,HIGH);
        offset = 0;
    }
      if(digitalRead(FIREbutton) == LOW){// check fire button for activity
        digitalWrite(discharge_LED,LOW);
        temp_val = FireintheHole();  //fire function
        if( temp_val == true){ //if it passes, return to beginning state.
         state = 1;
         firstpass = 1; 
        }
        else if(temp_val == false){//if it fails, turn the leds back on.
          digitalWrite(discharge_LED,HIGH);
          digitalWrite(fire_LED,HIGH); 
        }
        
      }
    
      else if(digitalRead(Disbutton) == LOW){ //check discharge button for activity
         digitalWrite(fire_LED,LOW);
         temp_val = DischargeSafe(); //discharge function
         if( temp_val == true){ //if it passes, return to beginning state.
           state = 1;
           firstpass = 1; 
         }
         else if(temp_val == false){ //if it fails, turn the LEDs back on.
           digitalWrite(discharge_LED,HIGH);
           digitalWrite(fire_LED,HIGH); 
         }
      }
      if(millis()- timeout - offset >= 1000){//ask for the current level of charge every 1s             
         offset = offset + 1000; //sure that the if statement isn't always true.
         askFunc();
      }
    
      if(millis() - timeout >= 20000){ //check timeout, go to sleep
         enterSleep();//go to sleep
      }
        
    break;
    
  } 
}


void enterSleep(){
 
   //turn off all LEDs
   turnOn(10);
   digitalWrite(charge_LED, LOW); 
   digitalWrite(fire_LED, LOW);
   //turn on discharge LED
   digitalWrite(discharge_LED, HIGH); 
   attachInterrupt(0, wakeUp, LOW); // sets up interrupt to wake up
   delay(10); //make sure interrupt is attached
   sleep_mode(); //sleep now
   //wakes up here 
   sleep_disable(); //disable sleep so that we dont go back                           
   detachInterrupt(0); //detaches interrupt so that normal code is unaffected
   delay(10);
   firstpass = 1; //reset flag
   state = 1; //go to beginning
   digitalWrite(discharge_LED, LOW); //turn off discharge LED ring
}

void wakeUp(){
  //do nothing
}

//funciton to send fire command and blink leds
int FireintheHole(){
  temp_val = SendandConfirm(fire_comm,0,2000);
  if(temp_val == false){
        //blink error led
        ErrorFunc();
        return false;
   }
   else if(temp_val == true){
     BlinkFunc(fire_LED,250,2); //blinks the fire buttonLED
   }
   return true;
}


//funtion to send discharge command and blink leds
int DischargeSafe(){
  temp_val = SendandConfirm(discharge_comm,0,2000);
  if(temp_val == false){
        //blink error led
        ErrorFunc();
        return false;
  }
  else if (temp_val == true){
    BlinkFunc(discharge_LED,250,2); //blinks the discharge buttonLED
    return true;
  }
  
}

//little function to ask for the current level of charge and update the charli LEDs
void askFunc(){
  if(SendandConfirm(askforcharge_comm,0,200) == true){
     temp_val = ReceiveandConfirm(500);
     turnOn((temp_val - 1)/10);
  }
}



//Charliplex LED controller. First sets up the pins(input,output) then sets their state(on,off)
void turnOn( int led ) {
    pinMode( A, matrix[led][PIN_CONFIG][0] ); 
    pinMode( B, matrix[led][PIN_CONFIG][1] );
    pinMode( C, matrix[led][PIN_CONFIG][2] );
    pinMode( D, matrix[led][PIN_CONFIG][3] );
    digitalWrite( A, matrix[led][PIN_STATE][0] );
    digitalWrite( B, matrix[led][PIN_STATE][1] );
    digitalWrite( C, matrix[led][PIN_STATE][2] );
    digitalWrite( D, matrix[led][PIN_STATE][3] );
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


//function to read incoming messages




//function to read incoming messages. Pass in struct to save a good message to and a timeout in ms to wait
//returns true if a valid new message came in and was saved to struct
//returns false if no message was received before timeout. 
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

int SendandConfirm( byte command, byte value, long timeout){
  struct myMessage currentMessage; //struct to hold the current message

  sendCommand(command,value); //sends command
  delay(SendDelay); //necessary to allow the signal to send. 
  if(readCommand(currentMessage, timeout) == true){//gets new message and checks CS and header
     //state check
     if(currentMessage.comm == command && currentMessage.val == value){ // checks command and value
       if(currentMessage.currentstate != state){ //quick state check, if they are different change the state to the real state.
        state = currentMessage.currentstate;
        firstpass = 1; // update flag on state change.
        SyncState();
        return 7; //dummy return
       }
       sendCommand(confirm_comm,0); //sends confirm message
       delay(SendDelay);
       return true;// if message was confirmed
     } 
  } 
  else{
     return false;//if message wasn't confirmed
  }
  return false; //to confirm that the message wasn't confirmed
}


	
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
                       if(previousMessage.comm == done_comm){ //checks to see if the message contained done command
                         return done_comm; // used to return the findings of receiveandconfirm
                       }
                       else if(previousMessage.comm == notdone_comm){//checks to see if the message contained charge
                         return previousMessage.val; //returns the value set charli leds to
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

int doneFunc(){ //read incoming data to update charli LEDS or exit loop becasue the done signal was received
      boolean done = false; //flag for waiting for done command
      byte value = 200; //temp place holder 200 is dummy command, does nothing
      byte hold_led = 100; // var used later to hold the highest led on during charging state.
      offset = 0; //place to store a number
      unsigned long int currentTime; //take current time for a little time out loop
      digitalWrite(discharge_LED,HIGH); //So the user know the discharge function can be used while charging.
      
      while(millis() - timeout < 60000){//wait for done command for 60 sec. 
         BlinkFunc(charge_LED,100,1);
          //need to poll the diacharge button to see if we need to discharge. However if youre pressing the button now then something bad is about to happen..
         if(digitalRead(Disbutton) == LOW){ // so we need to be sure this message gets through. dont exit until it goes through.
           temp_val = DischargeSafe(); //send discharge signal.
           if(temp_val == true){ //if it passes, return
             return false;
           }
           else if(temp_val == false){ //if it fails turn the light back on.
             digitalWrite(discharge_LED,HIGH);
           }
         }
       if(millis()- timeout - offset >= time_Calibration){             
         offset = offset + time_Calibration; //sure that the if statement isn't always true. 
          temp_val = SendandConfirm(askfordone_comm,0,200); 
         if(temp_val == true){
           value = ReceiveandConfirm(1000); //receive command and store the value received.
           if(value == done_comm){//check to see if command was done command
              return true; // the command was the done command, exit the loop
           }
         }
         else if (temp_val == 7){
           return false;
         }
       }
         
       if(value <= 100 && value > 0){ //check for valid value, also use to update charli
         while(value <= hold_led){ //finds the highest LED value corresponding to the sensorvalue.by decrementing through the LEDs
            hold_led--;  
          }
          turnOn(hold_led/10);  //turns on the highest LED.
          hold_led = 100; //reset var for next time
         }
         
      }
      //if we made it here we timed out. 
         enterSleep();//GO to SLEEP
         return false;
}

//function used to give visual feedback that the states were out of sync, and that the states are being synced.
void SyncState(){
   digitalWrite(charge_LED, LOW); 
   digitalWrite(discharge_LED, LOW);
   digitalWrite(fire_LED, LOW);
   delay(100);
   for( int i = 0 ; i < 2;i++){
   digitalWrite(charge_LED, HIGH);
   delay(100); 
   digitalWrite(discharge_LED, HIGH);
   delay(100);
   digitalWrite(fire_LED, HIGH);
   delay(100);
   digitalWrite(charge_LED, LOW); 
   digitalWrite(discharge_LED, LOW);
   digitalWrite(fire_LED, LOW);
   delay(100);
   }
}

//function to blink all LEDs to indicate ERROR
void ErrorFunc(){
  for( int temp = 0 ; temp < 2 ; temp++){
   digitalWrite(charge_LED, HIGH); 
   digitalWrite(discharge_LED, HIGH);
   digitalWrite(fire_LED, HIGH);
   delay(250);
   digitalWrite(charge_LED, LOW); 
   digitalWrite(discharge_LED, LOW);
   digitalWrite(fire_LED, LOW);
   delay(250);
  }
}

//function to blink a single LED
void BlinkFunc(byte LED, byte length, byte num){//takes the LED to blink, how long each blink is, and the number of times to blink it.
   for( int temp = 0 ; temp < num ; temp++){
    digitalWrite(LED,HIGH);
    delay(length);
    digitalWrite(LED,LOW);
    delay(length);
   }
}
