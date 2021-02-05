#include "defines.h"
#include <Arduino.h>
// #include <FlexCAN_T4.h>
#include "globalVars.h"
#include <eXoCAN.h>
#include  "canMessages.h"
#include "checksums.h"
#include "EPStoLKAS.h"
#include "LKAStoEPS.h"
#include "struct.h"
#include "sendSerial.h"



//called from the main loop.  reads only when needed by the TIME_BETWEEN_DIGIAL_READS define, in milliseconds
//this helps from too many digitalRead calls slowing down reading from the other buses.
//reads all Pushbuttons and DIP switches to set them into a variable.  
//also reads the POT
void handleInputReads(){
	if( ( millis() - lastDigitalReadTime ) > TIME_BETWEEN_DIGITIAL_READS){

		if( (millis() - OPTimeLastCANRecieved) < 1000 ) mainLedBlinkTimer = 500;
		else mainLedBlinkTimer = 2000; 
		
		digitalWrite(BLUE_LED,( EPStoLKASBuffer[2] >> 2 ) & B00000001);
		
		if(LkasFromCanChecksumErrorCount > 2){
			OPSteeringControlMessageStatusPending = false;
			LkasFromCanFatalError = true;
			LkasFromCanStatus = 2;
		} else if(!LkasFromCanFatalError) LkasFromCanChecksumErrorCount = 0;


		if(	(millis() - OPTimeLastCANRecieved) > 50 ){
			OPSteeringControlMessageStatusPending = false;
			LkasFromCanFatalError = true;
			LkasFromCanStatus = 1;
		}

		lastDigitalReadTime = millis();
	} // end if true
}


                    /*************** S E T U P ***************/
void setup() {
	EPStoLKAS_Serial.begin(9600,SERIAL_8E1);
	LKAStoEPS_Serial.begin(9600,SERIAL_8E1);
	// outputSerial.begin(OUTPUTSERIAL_BAUD);
	// pinMode(analogRotaryInputPin,INPUT);
	// pinMode(PB1_spoofLKASLeft, INPUT_PULLUP);
	// pinMode(PB2_spoofLKASRight, INPUT_PULLUP);
	// pinMode(PB3_spoofLKASStop, INPUT_PULLUP);
	// pinMode(PB4_spoofLKASSteerWithPOTEnablePin, INPUT_PULLUP);
	// pinMode(A1_applySteeringPotPin, INPUT_PULLUP);
	// pinMode(DIP1_spoofFullMCU, INPUT_PULLUP);
	// pinMode(DIP2,INPUT_PULLUP);
	// pinMode(DIP5,INPUT_PULLUP); // **disabled, removed from code** DIP 5 is used as a test to disable LIN output...
	// pinMode(DIP7_SpoofSteeringWheelTorqueData_PIN, INPUT_PULLUP);
	pinMode(BLUE_LED,OUTPUT);
	pinMode(LED_BUILTIN, OUTPUT);
	// canSetup();
	// FCAN.begin();
	// FCAN.setBaudRate(500000);
	can.filterMask16Init(0, 0xe4, 0x7ff); //TODO: confirm this mask is done correctly.. what is  filter 0 and filter 1?

}
int zeroVal = 0;

                    /*************** L O O P ***************/
void loop() {
  handleLKAStoEPS();
  handleEPStoLKAS();
  
  if(can.receive(zeroVal,canMsg.id,canMsg.buf)){
		handleLkasFromCanV3();
	}
  

}