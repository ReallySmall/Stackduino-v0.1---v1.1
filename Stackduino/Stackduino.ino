/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  STACKDUINO 1                                                                                        //
//                                                                                                      //
//  A sketch to drive an Arduino compatible MacroPhotography Focus Stacking Controller                  //
//  http://reallysmall.github.io/Stackduino/                                                            //
//                                                                                                      //
//  KEY HARDWARE:                                                                                       //
//  ATMega 328                                                                                          //
//  Easydriver                                                                                          //
//  Bipolar stepper motor                                                                               //
//  Rotary encoder with momentary push button                                                           //
//  16 x 2 HD44780 lcd                                                                                  //
//  2 x 4n35 optocoupler for camera connection                                                          //
//                                                                                                      //
//  CODE CREDIT:                                                                                        //
//  Reading a rotary encoder - http://www.circuitsathome.com/mcu/reading-rotary-encoder-on-arduino      //   
////////////////////////////////////////////////////////////////////////////////////////////////////////*/

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  LIBRARY DEPENDANCIES                                                                                //         
////////////////////////////////////////////////////////////////////////////////////////////////////////*/
#include <LiquidCrystal.h> //LCD library
LiquidCrystal lcd(8, 9, 10, 11, 12, 13); //lcd pins

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ARDUINO PIN ASSIGNMENTS                                                                             //
////////////////////////////////////////////////////////////////////////////////////////////////////////*/

#define ENC_A A0 //rotary encoder (14)
#define ENC_B A1 //rotary encoder (15)
#define ENC_PORT PINC //rotary encoder (a port not a pin)


const byte main_button = 2;  //start/ stop stack button
const byte encoder_button = 3; //select/ unselect menu item button
const byte set_direction = 4; //stepper motor direction
const byte do_step = 5; //send step signal to stepper motor
const byte focus = 6; //send an autofocus signal to the camera
const byte shutter = 7; //send a shutter signal to the camera
const byte forward_control = 16; //for manual positioning
const byte enable = 17; //enable and disable easydriver when not stepping to reduce heat and power consumption (future functionality)
const byte limit_switches = 18; //limit switches to stop stepping if end of travel on rail is reached at either end
const byte backward_control = 19; //for manual positioning

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  DEFAULT SETTINGS                                                                                    // 
////////////////////////////////////////////////////////////////////////////////////////////////////////*/
int bracket = 1; //number of images to bracket per focus slice
boolean disable_easydriver = true; //whether to disable easydriver betweem steps to save power and heat
int encoder_counter = 0; //count pulses from encoder
int encoder_pos = 1; //which menu item to display when turning rotary encoder
boolean forward = true; //store the current direction of travel - used when a limit switch is hit because we can't digitalRead the direction pin as it is set as an OUTPUT
int mirror_lockup = 1; //set to 2 to enable mirror lockup
int return_to_start = 1; //whether camera/ subject is returned to starting position at end of stack - set to 2 for yes
int set_pause = 5000; //default time in millis to wait for camera to take picture
int slice_counter = 0; //count of number of pictures taken so far
int slice_depth = 10; //default number of microns stepper motor should make between pictures
int slices = 10; //default total number of pictures to take
int stepper_speed = 2000; //delay in microseconds between motor steps, governing motor speed in stack
int unit_of_measure = 1; //whether to use microns, mm or cm when making steps
int unit_of_measure_multiplier = 1; //multiplier to factor into step signals (1, 1000 or 10,000) depending on unit of measure used
boolean update_screen = true; //whether to print to the lcd with new or updated information

//main button toggle
volatile int main_button_state = HIGH; //the current state of the output pin
volatile int main_button_reading; //the current reading from the input pin
volatile int main_button_previous = LOW; //the previous reading from the input pin
volatile long main_button_time = 0; //the last time the output pin was toggled
volatile long main_button_debounce = 400; //the debounce time, increase if the output flickers

//rotary pushButton toggle
volatile int rotary_button_state = HIGH; //the current state of the output pin
volatile int rbreading; //the current reading from the input pin
volatile int rbprevious = LOW; //the previous reading from the input pin
volatile long rbtime = 0; //the last time the output pin was toggled
volatile long rbdebounce = 400; //the debounce time, increase if the output flickers

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  SET PIN FUNCTIONS AND INITIAL STATES, START SERVICES AND PRINT STARTUP MESSAGE                      //    
////////////////////////////////////////////////////////////////////////////////////////////////////////*/
void setup() 

{

  lcd.begin(16, 2);
  Serial.begin(9600);  

  attachInterrupt(0, button_main_change, CHANGE);  //main button on interrupt 0
  attachInterrupt(1, button_rotary_change, CHANGE);  //encoder button on interrupt 1

  pinMode(main_button, INPUT);
  pinMode(encoder_button, INPUT);
  pinMode(set_direction, OUTPUT);   
  pinMode(do_step, OUTPUT);
  pinMode(focus, OUTPUT); 
  pinMode(shutter, OUTPUT);  
  pinMode(ENC_A, INPUT);
  pinMode(ENC_B, INPUT);
  pinMode(forward_control, INPUT); 
  pinMode(enable, OUTPUT);
  pinMode(limit_switches, INPUT);
  pinMode(backward_control, INPUT); 
   
  digitalWrite(main_button, HIGH);
  digitalWrite(encoder_button, HIGH);
  digitalWrite(set_direction, HIGH);
  digitalWrite(do_step, LOW);
  digitalWrite(focus, LOW);
  digitalWrite(shutter, LOW);
  digitalWrite(ENC_A, HIGH);
  digitalWrite(ENC_B, HIGH);
  digitalWrite(forward_control, HIGH);
  digitalWrite(enable, LOW);
  digitalWrite(limit_switches, HIGH);
  digitalWrite(backward_control, HIGH);

  lcd.setCursor(0, 0);
  lcd.print("Stackduino");
  delay(1000);
  lcd.clear();

}

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  BUTTON FUNCTIONS                                                                                    //       
////////////////////////////////////////////////////////////////////////////////////////////////////////*/ 

void button_main_change(){ /* RETURN CURRENT STATE OF MAIN PUSH BUTTON */

  main_button_reading = digitalRead(main_button);

  if (main_button_reading == LOW && main_button_previous == HIGH && millis() - main_button_time > main_button_debounce) {
    if (main_button_state == HIGH)
    main_button_state = LOW;
    else
    main_button_state = HIGH;

    main_button_time = millis();    
  }

  main_button_previous = main_button_reading;
} 

void button_rotary_change(){/* RETURN CURRENT STATE OF ROTARY ENCODER'S PUSH BUTTON */

  rbreading = digitalRead(encoder_button);
  if (rbreading == LOW && rbprevious == HIGH && millis() - rbtime > rbdebounce) {
    if (rotary_button_state == HIGH)
    rotary_button_state = LOW;
    else
    rotary_button_state = HIGH;
    rbtime = millis();    
  }

  rbprevious = rbreading;

}

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  CAMERA INTERFACE FUNCTIONS                                                                          //       
////////////////////////////////////////////////////////////////////////////////////////////////////////*/ 

void camera_process_images(){ /* SEND SIGNAL TO CAMERA TO TAKE PICTURE WITH DELAYS TO ALLOW SETTLING */
  for (int i = 1; i <= bracket; i++)
  {
    lcd.clear();
    if(bracket > 1){ //if bracketing is enabled, print the current position in the bracket
      lcd.print("Bracketed image:");
      lcd.setCursor(0, 1);
      lcd.print(i);
      lcd.print(" of ");
      lcd.print(bracket);
      delay(1000);
      lcd.clear();
    }
    
    camera_shutter_signal(); //send a shutter signal
       
    lcd.print("Pause for image");
    lcd.setCursor(0, 1);
    lcd.print("(");
    lcd.print ((set_pause / 1000), DEC);
    lcd.print(" seconds)");

    delay(set_pause); //pause to allow for camera to take picture and to allow flashes to recharge before next shot

    lcd.clear();
  }
}

void camera_shutter_signal() { /* SEND SHUTTER SIGNAL */ 
  for (int i = 0; i < mirror_lockup; i++)
  {

    if(mirror_lockup == 2 && i == 0){
      lcd.clear();
      lcd.print("Mirror up");
    }

    digitalWrite(focus, HIGH); //trigger camera autofocus - camera may not take picture in some modes if this is not triggered first
    digitalWrite(shutter, HIGH); //trigger camera shutter

    delay(200); //small delay needed for camera to process above signals

    digitalWrite(shutter, LOW); //switch off camera trigger signal
    digitalWrite(focus, LOW); //switch off camera focus signal

    if(mirror_lockup == 2 && i == 0){
      delay(2000); //sets the delay between mirror up and shutter
      lcd.clear();
    }
  }
}

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ROTARY ENCODER FUNCTIONS                                                                            //       
////////////////////////////////////////////////////////////////////////////////////////////////////////*/  

int8_t encoder_read(){ /* RETURN CHANGE IN ENCODER STATE */

  static int8_t enc_states[] = { 
    //0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0 //use this line instead to increment encoder in the opposite direction
    0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0
  };
  static uint8_t old_AB = 0;
  /**/
  old_AB <<= 2; //remember previous state
  old_AB |= ( ENC_PORT & 0x03 ); //add current state
  return ( enc_states[( 0x0f & old_AB )]);

}

void encoder_update(int &variable, int lower, int upper, int multiplier = 1){ /* CHANGE THE ACTIVE MENU VARIABLE'S VALUE USING THE ENCODER */
  
  variable = constrain(variable, lower, upper); //keep variable value within specified range  
  
  int8_t encoder_data = encoder_read(); //counts encoder pulses and registers only every nth pulse to adjust feedback sensitivity
  
  if(encoder_data){
    if(encoder_data == 1){
      encoder_counter++;
    }

    if(encoder_counter == 2){ //change this number to adjust encoder sensitivity 4 or 2 are usually the values to use
      variable = variable + multiplier;
      update_screen = true; //as a variable has just been changed by the encoder, flag the screen as updatable
      encoder_counter = 0;
    }

    if(encoder_data == -1){
      encoder_counter--;
    }
    if(encoder_counter == -2){ //change this number to adjust encoder sensitivity 4 or 2 are usually the values to use
      variable = variable - multiplier;
      update_screen = true; //as a variable has just been changed by the encoder, flag the screen as updatable
      encoder_counter = 0;
    }
  }

}

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  LCD DISPLAY FORMATTING FUNCTIONS                                                                    //       
////////////////////////////////////////////////////////////////////////////////////////////////////////*/  

void lcd_front_load(int variable) { /* FRONT PAD MENU ITEM NUMBERS WITH ZEROES */
  if (variable < 10){
    lcd.print (000, DEC); //adds three leading zeros to single digit Step size numbers on the display
  }
  if (variable < 100){
    lcd.print (00, DEC); //adds two leading zeros to double digit Step size numbers on the display
  }
  if (variable < 1000){
    lcd.print (0, DEC); //adds one leading zero to triple digit Step size numbers on the display
  }
  lcd.print(variable, DEC);
}

void lcd_unit_of_measure() { /* PRINT SELECTED UNIT OF MEASURE TO SCREEN */        
  if (unit_of_measure==1){
    lcd.print(" mn");
  }
  if (unit_of_measure==2){
    lcd.print(" mm");
  }
  if (unit_of_measure==3){
    lcd.print(" cm");
  }
}

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  STEPPER MOTOR CONTROL FUNCTIONS                                                                     //       
////////////////////////////////////////////////////////////////////////////////////////////////////////*/

void stepper_control(){ /* MOVE CARRIAGE BACKWARD AND FORWARD MANUALLY USING PUSH BUTTONS */

  while (digitalRead(forward_control) == LOW) {
    stepper_enable();
    digitalWrite(set_direction, HIGH);
    forward = true; 
    for (int i = 0; i<1; i++)
    {
      stepper_step(); //move forward
    }
    stepper_disable();
  }

  while (digitalRead(backward_control) == LOW) {
    stepper_enable();
    digitalWrite(set_direction, LOW);
    forward = false; 
    for (int i = 0; i<1; i++)
    {
      stepper_step(); //move backward
    }
    stepper_disable();
  }

}

void stepper_direction(){ /* REVERSE THE CURRENT STEPPER DIRECTION */
  if(forward){
      digitalWrite(set_direction, LOW);
      forward = false;
    } else {
      digitalWrite(set_direction, HIGH);
      forward = true;
    }
}

void stepper_disable() { /* DISABLE THE EASYDRIVER WHEN NOT IN USE IF OPTION SET */
  if(disable_easydriver == true) {
    digitalWrite(enable, HIGH);
  } 
  else {
    digitalWrite(enable, LOW);
  } 
}

void stepper_enable() { /* ENABLE THE EASYDRIVER */
  digitalWrite(enable, LOW);
}

void stepper_retreat(){ /* PULL CARRIAGE BACK FROM TRIPPED LIMIT SWITCH */
  stepper_enable();
  stepper_direction(); //switch the motor direction
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("End of travel!");
  lcd.setCursor(0, 1);
  lcd.print("Reversing...");

  while (digitalRead(limit_switches) == LOW) //iterate do_step signal for as long as eitherlimit switch remains pressed 
  {  
    stepper_step();
  }

  stepper_direction(); //reset motor back to original direction once limit switch is no longer pressed
  lcd.clear();
  stepper_disable();
}

void stepper_step(){ /* SEND A STEP SIGNAL TO EASYDRIVER TO TURN MOTOR */

  digitalWrite(do_step, LOW); //this LOW to HIGH change is what creates the
  digitalWrite(do_step, HIGH); //"Rising Edge" so the easydriver knows to when to step
  delayMicroseconds(stepper_speed); //delay time between steps, too fast and motor stalls

}

void loop(){

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  CHECK A LIMIT SWITCH ISN'T TRIPPED ON STARTUP                                                       //     
////////////////////////////////////////////////////////////////////////////////////////////////////////*/
  if (digitalRead(limit_switches) == LOW){ //if controller is started up hitting a limit switch disable main functions and print warning to lcd
    stepper_disable(); //disable easydriver

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("End of travel!");
    lcd.setCursor(0, 1); 

    stepper_control(); //Provide manual motor control to move away from limit switch and resolve error
  }
  else{ //if limit switches HIGH run all functions normally

    if (main_button_state == HIGH){

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  MENUS AND MANUAL FWD/ BWD CONTROL                                                                   //  
////////////////////////////////////////////////////////////////////////////////////////////////////////*/

      stepper_disable(); //switch off stepper motor power if option enabled
      stepper_control(); //manual motor control to position stage before stack

      if (rotary_button_state == HIGH) { //use encoder to scroll through menu of settings

        encoder_update(encoder_pos, 0, 8);

        if (encoder_pos == 9){ //when counter value exceeds number of menu items
          encoder_pos = 1; //reset it to 1 again to create a looping navigation
        }

        if (encoder_pos == 0){ //when counter value goes below minimum number of menu items
          encoder_pos = 8; //reset it to 7 again to create a looping navigation
        }   

      } 

      switch (encoder_pos) { //the menu options

      case 1: //this menu screen changes the distance to move each time
        if (rotary_button_state == LOW) { //if rotary encoder button is toggled within a menu, enabled editing of menu variable with encoder
          encoder_update(slice_depth, 1, 1000);
        }

        if (update_screen){ //the screen hardware is slow so only print to it when something changes
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Set step size:");
          lcd.setCursor(0, 1);
          lcd_front_load(slice_depth); //then use that figure to frontload with correct number of zeroes and print to screen
          lcd_unit_of_measure();
          update_screen = false;
        }      

        break;

      case 2: //this menu screen changes the number of pictures to take in the stack
        if (rotary_button_state == LOW) { //if rotary encoder button is toggled within a menu, enabled editing of menu variable with encoder
          encoder_update(slices, 10, 5000, 10);
        }

        if (update_screen){  //the screen hardware is slow so only print to it when something changes
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Set num steps:");
          lcd.setCursor(0, 1);
          lcd_front_load(slices); //frontload with correct number of zeroes and print to screen
          update_screen = false;
        } 

        break;

      case 3: //this menu screen changes the number of seconds to wait for the camera to take a picture before moving again - 
        //you may want longer if using flashes for adequate recharge time or shorter with continuous lighting
        //to reduce overall time taken to complete the stack
        if (rotary_button_state == LOW) { //if rotary encoder button is toggled within a menu, enabled editing of menu variable with encoder
          encoder_update(set_pause, 1000, 30000, 1000);
        }

        if (update_screen){  //the screen hardware is slow so only print to it when something changes
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Set pause time:");
          lcd.setCursor(0, 1);
          if (set_pause < 10000){
            lcd.print (0, DEC); //adds one leading zero to triple digit set_pause numbers on the display
          }
          lcd.print ((set_pause / 1000), DEC); //divide millis by 1000 to display in seconds
          lcd.print(" seconds");  
          update_screen = false;
        }

        break;

      case 4: //toggles whether camera/subject is returned the starting position at the end of the stack
        if (rotary_button_state == LOW) { //if rotary encoder button is toggled within a menu, enabled editing of menu variable with encoder
          encoder_update(return_to_start, 1, 2);
        }

        if (update_screen){  //the screen hardware is slow so only print to it when something changes

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Return to start:");
          lcd.setCursor(0, 1);
          if(return_to_start == true){
            lcd.print ("Enabled");
          }
          else {
            lcd.print ("Disabled");
          }
          update_screen = false;

        }

        break; 

      case 5: //this menu screen selects the unit of measure to use for steps: Microns, Millimimeters or Centimeteres
        if (rotary_button_state == LOW) { //if rotary encoder button is toggled within a menu, enabled editing of menu variable with encoder
          encoder_update(unit_of_measure, 1, 3);
        }

        if (update_screen == true){  //the screen hardware is slow so only print to it when something changes

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Unit of measure:");
          lcd.setCursor(0, 1);

          switch (unit_of_measure){ 

            case 1:

            lcd.print ("Microns (mn)");
            unit_of_measure_multiplier = 1;
            break;

            case 2:

            lcd.print ("Millimetres (mm)");
            unit_of_measure_multiplier = 1000;
            break;

            case 3: 

            lcd.print ("Centimetres (cm)");
            unit_of_measure_multiplier = 10000;
            break;
          }

          update_screen = false;

        }

        break; 

      case 6: //this menu screen adjusts the stepper motor speed (delay in microseconds between steps)
        //setting this too low (ie a faster motor speed) may cause the motor to begin stalling or failing to move at all
        if (rotary_button_state == LOW) { //if rotary encoder button is toggled within a menu, enabled editing of menu variable with encoder
          encoder_update(stepper_speed, 1000, 9000, 1000);
        }

        if (update_screen == true){  //the screen hardware is slow so only print to it when something changes

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Stepper speed:");
          lcd.setCursor(0, 1);
          lcd.print (stepper_speed, DEC);
          lcd.print (" microsecs");
          update_screen = false;

        }

        break; 

      case 7: //this menu screen changes the number of images to take per focus slice (ie supports bracketing)
        if (rotary_button_state == LOW) { //if rotary encoder button is toggled within a menu, enabled editing of menu variable with encoder
          encoder_update(bracket, 1, 10);
        }

        if (update_screen == true){  //the screen hardware is slow so only print to it when something changes

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Set bracketing:");
          lcd.setCursor(0, 1);
          lcd_front_load(bracket); //then use that figure to frontload with correct number of zeroes and print to screen       
          update_screen = false;

        }      

        break;

      case 8: //this menu screen toggles mirror lockup mode - if enabled two delay-seperated shutter signals are sent per image
        if (rotary_button_state == LOW) { //if rotary encoder button is toggled within a menu, enabled editing of menu variable with encoder
          encoder_update(mirror_lockup, 1, 2);
        }

        if (update_screen == true){  //the screen hardware is slow so only print to it when something changes

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Mirror Lockup:");
          lcd.setCursor(0, 1);
          if(mirror_lockup == 1){
            lcd.print ("Disabled");  
              } else {
                lcd.print ("Enabled");  
              }          
              update_screen = false;

            }      

            break;

          }
    } //end of setup menu section

    else {

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  THE FOCUS STACK                                                                                     //     
////////////////////////////////////////////////////////////////////////////////////////////////////////*/

      stepper_disable();

      for (int i = 0; i < slices; i++){ //loop the following actions for number of times dictated by var slices

        slice_counter++; //count of pictures taken so far

        lcd.clear();
        lcd.print("Moving ");
        lcd.print (slice_depth);
        lcd_unit_of_measure();
        lcd.setCursor(0, 1);
        lcd.print("Step ");
        lcd.print (slice_counter);
        lcd.print (" of ");
        lcd.print (slices);

        delay(500);
        stepper_enable();
        digitalWrite(set_direction, HIGH); //set the stepper direction for forward travel
        delay(100);

        int i = 0; //counter for motor steps
        while (i < slice_depth * 16 * unit_of_measure_multiplier && digitalRead(limit_switches) == HIGH){ //adjust the number in this statement to tune distance travelled on your setup. In this case 16 is a product of 8x microstepping and a 2:1 gearing ratio
          stepper_step();
          i++;
        }
        i = 0; //reset counter
        stepper_disable();

        if (digitalRead(limit_switches) == LOW){ //stop motor and reverse if limit switch hit
          stepper_retreat();
          break;
        }    

        if (main_button_state == HIGH){ //if the Start/Stop stack button has been pressed, stop the stack even if not complete
          break;
        }

        camera_process_images(); //send signal to camera to take picture(s)

        if (main_button_state == HIGH){ //if the Start/Stop stack button has been pressed, stop the stack even if not complete
          break;
        }
      } 
        lcd.setCursor(0, 0);
        lcd.print("Stack finished");
        delay(2000);
        lcd.clear(); 

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  IF ENABLED, AT END OF STACK RETURN TO START POSITION                                                //      
////////////////////////////////////////////////////////////////////////////////////////////////////////*/        
      if (return_to_start == 2){   
        digitalWrite(set_direction, LOW); //set the stepper direction for backward travel
        delay(100);
        lcd.setCursor(0, 0);
        lcd.print("<< Returning..."); 
        int returnSteps = slice_depth * slice_counter;
        lcd.setCursor(0, 1);
        lcd.print (returnSteps);
        lcd_unit_of_measure();

        stepper_enable();

        int i = 0; //counter for motor steps
        while (i < returnSteps * 16 * unit_of_measure_multiplier && digitalRead(limit_switches) == HIGH){
          stepper_step();
          i++;
        }
        i = 0; //reset counter

        stepper_disable();

        if (digitalRead(limit_switches) == LOW){ //stop motor and reverse if limit switch hit
          stepper_retreat();
        }  

        lcd.clear();
      }

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  RESET VARIABLES TO DEFAULTS BEFORE RETURNING TO MENU AND MANUAL CONTROL                             //                                                                              
////////////////////////////////////////////////////////////////////////////////////////////////////////*/  

      encoder_pos = 1; //set menu option display to first
      update_screen = true; //allow the first menu item to be printed to the screen
      slice_counter = 0; //reset pic counter
      main_button_state = HIGH; //return to menu options
    } 
  }
}
