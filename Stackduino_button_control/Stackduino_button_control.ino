/*
 Stackduino
 
 A sketch to drive an Arduino based MacroPhotography Focus Stacking Controller
 http://www.flickr.com/photos/reallysmall/sets/72157632341602394/
 
 Key parts:
 Arduino
 Easydriver
 Bipolar stepper motor
 Rotary encoder with momentary push button
 16 x 2 HD44780 lcd
 2 x 4n35 optocoupler for camera connection
 
 Key resources used:
 Easydriver tutorial - http://danthompsonsblog.blogspot.com/2010/05/easydriver-42-tutorial.html
 Rotary encoder code - http://www.circuitsathome.com/mcu/reading-rotary-encoder-on-arduino
 */

#include <LiquidCrystal.h> //LCD library
#include <DigitalToggle.h> //digitalWrite toggle library

LiquidCrystal lcd(8, 9, 10, 11, 12, 13); //lcd pins

//rotary encoder pins
#define ENC_A A0
#define ENC_B A1
#define ENC_PORT PINC

int steps = 10; //number of microns stepper motor should make between pictures, default 12
int numPictures = 10; //total number of pictures to take
int picCounter = 0; //count of number of pictures taken so far
int setPause = 5000; //time in millis to wait for camera to take picture
int returnToStart = 2; //whether camera/ subject is returned to starting position at end of stack, controlled by returnNum
int manualSpeedXaxis = 0; //delay between steps in manual mode, governing motor speed
int joyStickreading = 0; //current analogue reading of joystick
int rotaryCounter = 1; //which menu item to display when turning rotary encoder
int unitofMeasure = 1; //whether to use microns, mm or cm when making steps
int uomMultiplier = 1; //multiplier to factor into step signals (1, 1000 or 10,000) depending on unit of measure used
int stepSpeed = 2000; //delay in microseconds between motor steps, governing motor speed in stack
int lcdloopCounter = 0; //count number of loops to periodically update lcd

int pushButton = 2;  //start/ stop stack button
int rotarypushButton = 3; //select/ unselect menu item button
int dir = 4; //stepper motor direction
int doStep = 5; //send step signal to stepper motor
int focus = 6; //send an autofocus signal to the camera
int shutter = 7; //send a shutter signal to the camera
int forwardControl = 16; //for manual positioning
int backwardControl = 17; //for manual positioning
int limitSwitches = 19; //limit switches to stop stepping if end of travel on rail is reached at either end

//pushButton toggle
volatile int buttonState = HIGH; //the current state of the output pin
volatile int reading; //the current reading from the input pin
volatile int previous = LOW; //the previous reading from the input pin
volatile long time = 0; //the last time the output pin was toggled
volatile long debounce = 400; //the debounce time, increase if the output flickers

//rotary pushButton toggle
volatile int rbbuttonState = HIGH; //the current state of the output pin
volatile int rbreading; //the current reading from the input pin
volatile int rbprevious = LOW; //the previous reading from the input pin
volatile long rbtime = 0; //the last time the output pin was toggled
volatile long rbdebounce = 400; //the debounce time, increase if the output flickers

void setup() 

{

  lcd.begin(16, 2);
  Serial.begin(9600);  

  attachInterrupt(0, buttonChange, CHANGE);  //pushButton on interrupt 0
  attachInterrupt(1, rotarybuttonChange, CHANGE);  //rotarypushButton on interrupt 1

  pinMode(pushButton, INPUT);
  pinMode(rotarypushButton, INPUT);
  pinMode(ENC_A, INPUT);
  pinMode(ENC_B, INPUT);
  pinMode(limitSwitches, INPUT);
  pinMode(dir, OUTPUT);   
  pinMode(doStep, OUTPUT);    
  pinMode(focus, OUTPUT); 
  pinMode(shutter, OUTPUT); 
  pinMode(forwardControl, INPUT); 
  pinMode(backwardControl, INPUT); 
  pinMode(limitSwitches, INPUT);

  digitalWrite(focus, LOW);
  digitalWrite(shutter, LOW);
  digitalWrite(ENC_A, HIGH);
  digitalWrite(ENC_B, HIGH);
  digitalWrite(pushButton, HIGH);
  digitalWrite(rotarypushButton, HIGH);
  digitalWrite(forwardControl, HIGH);
  digitalWrite(backwardControl, HIGH);
  digitalWrite(limitSwitches, HIGH);

  lcd.setCursor(0, 0);
  lcd.print("Stackduino");
  delay(3000);
  lcd.clear();

}

void loop(){

  if (digitalRead(limitSwitches) == LOW){ //if controller is started up hitting a limit switch disable main functions and print warning to lcd

    lcdloopCounter++;

    if (lcdloopCounter >= 40){

      lcd.setCursor(0, 0);
      lcd.print("End of travel!  ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcdloopCounter = 0;

    }      

    //Provide manual motor control to move away from limit switch and resolve error
    manualControl();

  }

  else{ //if limit switches HIGH run all functions normally

    if (buttonState == HIGH){ //this section allows manual control and configures settings using a simple lcd menu system

      //manual motor control to position stage before stack
      manualControl();

      if (rbbuttonState == HIGH) { //use encoder to scroll through menu of settings

        rotaryCounter = constrain(rotaryCounter, 0, 7); //limits choice to specified range

        rotaryCounter += read_encoder (); //use encoder reading function to get rotaryCounter value

        if (rotaryCounter == 7){ //when counter value exceeds number of menu items
          rotaryCounter = 1; //reset it to 1 again to create a looping navigation
        }

        if (rotaryCounter == 0){ //when counter value goes below minimum number of menu items
          rotaryCounter = 6; //reset it to 7 again to create a looping navigation
        }     

      } 

      switch (rotaryCounter) {

      case 1: //this menu screen changes the number of steps to move each time

        if (rbbuttonState == LOW) { //press rotary encoder button within this menu item to edit variable
          steps = constrain(steps, 1, 1000); //limits choice of input step size to specified range
          steps += read_encoder ();  //use encoder reading function to set value of steps variable
        }

        lcdloopCounter = lcdloopCounter + 1;

        if (lcdloopCounter >= 40){

          lcd.setCursor(0, 0);
          lcd.print("Set step size:  ");
          lcd.setCursor(0, 1);

          if (steps < 10){
            lcd.print (000, DEC); //adds three leading zeros to single digit Step size numbers on the display
          }
          if (steps < 100){
            lcd.print (00, DEC); //adds two leading zeros to double digit Step size numbers on the display
          }
          if (steps < 1000){
            lcd.print (0, DEC); //adds one leading zero to triple digit Step size numbers on the display
          }
          lcd.print (steps  , DEC);

          if (unitofMeasure==1){
            lcd.print(" mn");
          }
          if (unitofMeasure==2){
            lcd.print(" mm");
          }
          if (unitofMeasure==3){
            lcd.print(" cm");
          }
          lcd.print("         "); //fill rest of display line with empty chars to overwrite conetnt of previous screen  

          lcdloopCounter = 0;

        }      

        break;

      case 2: //this menu screen changes the number of pictures to take in the stack

        if (rbbuttonState == LOW) { //press rotary encoder button within this menu item to edit variable
          numPictures = constrain(numPictures, 10, 5000); //limits choice of number of pictures to take to specified range
          numPictures += (read_encoder () * 10); //use encoder reading function to set value of numPictures variable - 
          //changes in increments of 10 for quick selection of large numbers
        }

        lcdloopCounter = lcdloopCounter + 1;

        if (lcdloopCounter >= 40){

          lcd.setCursor(0, 0);
          lcd.print("Set num steps:  ");
          lcd.setCursor(0, 1);
          if (numPictures < 10){
            lcd.print (000, DEC); //adds three leading zeros to single digit numPictures numbers on the display
          }
          if (numPictures < 100){
            lcd.print (00, DEC); //adds two leading zeros to double digit numPictures numbers on the display
          }
          if (numPictures < 1000){
            lcd.print (0, DEC); //adds one leading zero to triple digit numPictures numbers on the display
          }
          lcd.print (numPictures, DEC);
          lcd.print ("        ");

          lcdloopCounter = 0;

        } 

        break;

      case 3: //this menu screen changes the number of seconds to wait for the camera to take a picture before moving again - 
        //you may want longer if using flashes for adequate recharge time or shorter with continuous lighting
        //to reduce overall time taken to complete the stack

        if (rbbuttonState == LOW) { //press rotary encoder button within this menu item to edit variable
          setPause = constrain(setPause, 1000, 30000); //limits choice of pause time between pictures to specified range
          setPause += (read_encoder () * 1000); //use encoder reading function to set value of setPause variable
        }

        lcdloopCounter = lcdloopCounter + 1;

        if (lcdloopCounter >= 40){

          lcd.setCursor(0, 0);
          lcd.print("Set pause time: ");
          lcd.setCursor(0, 1);

          if (setPause < 10000){
            lcd.print (0, DEC); //adds one leading zero to triple digit setPause numbers on the display
          }
          lcd.print ((setPause / 1000), DEC); //divide millis by 1000 to display in seconds
          lcd.print(" seconds      ");  

          lcdloopCounter = 0;

        }

        break;

      case 4: //toggles whether camera/subject is returned the starting position at the end of the stack

        if (rbbuttonState == LOW) { //press rotary encoder button within this menu item to edit variable
          returnToStart = constrain(returnToStart, 1, 2); //limits choice of returnToStart to specified range
          returnToStart += read_encoder (); //use encoder reading function to set value of returnToStart variable
        }

        lcdloopCounter = lcdloopCounter + 1;

        if (lcdloopCounter >= 40){

          lcd.setCursor(0, 0);
          lcd.print("Return to start:");
          lcd.setCursor(0, 1);

          if(returnToStart == 1){
            lcd.print ("Enabled         ");
          }
          else {
            lcd.print ("Disabled        ");
          }

          lcdloopCounter = 0;

        }

        break; 

      case 5: //this menu screen selects the unit of measure to use for steps: Microns, Millimimeters or Centimeteres

        if (rbbuttonState == LOW) { //press rotary encoder button within this menu item to edit variable
          unitofMeasure = constrain(unitofMeasure, 1, 3); //limits choice of returnToStart to specified range
          unitofMeasure += read_encoder (); //use encoder reading function to set value of returnToStart variable
        }

        lcdloopCounter = lcdloopCounter + 1;

        if (lcdloopCounter >= 40){

          lcd.setCursor(0, 0);
          lcd.print("Unit of measure:");
          lcd.setCursor(0, 1);

          switch (unitofMeasure){ 

          case 1:

            lcd.print ("Microns (mn)    ");
            uomMultiplier = 1;
            break;

          case 2:

            lcd.print ("Millimetres (mm)");
            uomMultiplier = 1000;
            break;

          case 3: 

            lcd.print ("Centimetres (cm)");
            uomMultiplier = 10000;
            break;
          }

          lcdloopCounter = 0;

        }

        break; 

      case 6: //this menu screen adjusts the stepper motor speed (delay in microseconds between steps)
        //setting this to low may cause the motor to begin stalling or failing to move at all

        if (rbbuttonState == LOW) { //press rotary encoder button within this menu item to edit variable
          stepSpeed = constrain(stepSpeed, 1000, 9000); //limits choice of returnToStart to specified range
          stepSpeed += (read_encoder () * 1000); //use encoder reading function to set value of stepSpeed variable
        }

        lcdloopCounter = lcdloopCounter + 1;

        if (lcdloopCounter >= 40){

          lcd.setCursor(0, 0);
          lcd.print("Stepper speed:  ");
          lcd.setCursor(0, 1);
          lcd.print (stepSpeed, DEC);
          lcd.print (" microsecs  ");

          lcdloopCounter = 0;

        }

        break; 
        
      }
    } //end of setup menu section

    else { //this section runs the stack when the pushButton is pressed

      for (int i = 0; i < numPictures; i++){ //loop the following actions for number of times dictated by var numPictures

        picCounter++; //count of pictures taken so far

        lcd.clear();
        lcd.print("Moving ");
        lcd.print (steps);
        if (unitofMeasure==1){
          lcd.print(" mn");
        }
        if (unitofMeasure==2){
          lcd.print(" mm");
        }
        if (unitofMeasure==3){
          lcd.print(" cm");
        }
        lcd.setCursor(0, 1);
        lcd.print("Step ");
        lcd.print (picCounter);
        lcd.print (" of ");
        lcd.print (numPictures);

        delay(500);

          digitalWrite(dir, LOW); //set the stepper direction to clockwise
          delay(100);

          int i = 0; //counter for motor steps
          while (i < steps * 8 * uomMultiplier && digitalRead(limitSwitches) == HIGH){
            stepSignal();
            i++;
          }
          i = 0; //reset counter

          if (digitalRead(limitSwitches) == LOW){ //stop motor and reverse if limit switch hit
            retreat();
            break;
          }    

          if (buttonState == HIGH){ //if the Start/Stop stack button has been pressed, stop the stack even if not complete
            break;
          }

            lcd.clear();
            lcd.print("Pause for image");
            lcd.setCursor(0, 1);
            lcd.print("(");
            lcd.print ((setPause / 1000), DEC);
            lcd.print(" seconds)");

            digitalWrite(focus, HIGH); //trigger camera autofocus - camera may not take picture in some modes if this is not triggered first
            digitalWrite(shutter, HIGH); //trigger camera shutter

            delay(200); //small delay needed for camera to process above signals

            digitalWrite(shutter, LOW); //switch off camera trigger signal
            digitalWrite(focus, LOW); //switch off camera focus signal

            delay(setPause); //pause to allow for camera to take picture with 2 sec mirror lockup and to allow flashes to recharge before next shot

            lcd.clear();

        if (buttonState == HIGH){ //if the Start/Stop stack button has been pressed, stop the stack even if not complete
          break;

        }
      } 

      lcd.setCursor(0, 0);
      lcd.print("Stack finished");
      delay(2000);
      lcd.clear(); 


      if (returnToStart == 1){   
        digitalWrite(dir, HIGH); //set the stepper direction to anti-clockwise
        delay(100);
        lcd.setCursor(0, 0);
        lcd.print("<< Returning..."); 

        int returnSteps = steps * picCounter;

        lcd.setCursor(0, 1);
        lcd.print (returnSteps);
        if (unitofMeasure==1){
          lcd.print(" mn");
        }
        if (unitofMeasure==2){
          lcd.print(" mm");
        }
        if (unitofMeasure==3){
          lcd.print(" cm");
        }

        int i = 0; //counter for motor steps
        while (i < returnSteps * 8 * uomMultiplier && digitalRead(limitSwitches) == HIGH){
          stepSignal();
          i++;
        }
        i = 0; //reset counter

        if (digitalRead(limitSwitches) == LOW){ //stop motor and reverse if limit switch hit
          retreat();
        }  

        lcd.clear();

      }

      rotaryCounter = 1; //set back to first screen of menu
      picCounter = 0; //reset pic counter
      buttonState = HIGH; //return to menu options

    } 
  }
}
//end of main code

void buttonChange(){ //function to read the current state of the push button

  reading = digitalRead(pushButton);

  if (reading == LOW && previous == HIGH && millis() - time > debounce) {
    if (buttonState == HIGH)
      buttonState = LOW;
    else
      buttonState = HIGH;

    time = millis();    
  }

  previous = reading;
}  


void rotarybuttonChange(){ //function to read the current state of the push button

  rbreading = digitalRead(rotarypushButton);

  if (rbreading == LOW && rbprevious == HIGH && millis() - rbtime > rbdebounce) {
    if (rbbuttonState == HIGH)
      rbbuttonState = LOW;
    else
      rbbuttonState = HIGH;

    rbtime = millis();    
  }

  rbprevious = rbreading;
} 

/* returns change in encoder state (-1,0,1) */
int8_t read_encoder()
{
  static int8_t enc_states[] = { 
    0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0
  };
  static uint8_t old_AB = 0;
  /**/
  old_AB <<= 2; //remember previous state
  old_AB |= ( ENC_PORT & 0x03 ); //add current state
  return ( enc_states[( 0x0f & old_AB )]);
}

void retreat(){

  digitalToggle(dir); //reverse motor direction to move away from limitswitch

  lcd.setCursor(0, 0);
  lcd.print("End of travel!  ");
  lcd.setCursor(0, 1);
  lcd.print("Reversing...    ");

  while (digitalRead(limitSwitches) == LOW) //iterate doStep signal for as long as eitherlimit switch remains pressed 
  {  
    stepSignal();
  }

  digitalToggle(dir); //reset motor back to original direction once limit switch is no longer pressed
  lcd.clear();
}

void manualControl(){
  
 if(digitalRead(forwardControl == LOW) && digitalRead(backwardControl == HIGH)){ //if only forward button pressed
    digitalWrite(dir, LOW); //set stepper direction to forward
    stepSignal(); //move forward
 }
 
 if(digitalRead(backwardControl == LOW) && (digitalRead(forwardControl == HIGH){ //if only backward button pressed
    digitalWrite(dir, HIGH); //set stepper direction to forward
    stepSignal(); //move backward
 }
  
}

void stepSignal(){
  
    digitalWrite(doStep, LOW); //this LOW to HIGH change is what creates the
    digitalWrite(doStep, HIGH); //"Rising Edge" so the easydriver knows to when to step
    delayMicroseconds(stepSpeed); //delay time between steps, too fast and motor stalls
  
}























