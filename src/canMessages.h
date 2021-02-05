#ifndef BUILDCANMESSAGESH
#define BUILDCANMESSAGESH


// #include <FlexCAN_T4.h>
#include <eXoCAN.h>
#include "struct.h"
#include "globalExtern.h"
#include "checksums.h"

void buildSteerMotorTorqueCanMsg();
void buildSteerStatusCanMsg();
void handleLkasFromCanV3();


// BO_ 427 STEER_MOTOR_TORQUE: 3 EPS
//  SG_ MOTOR_TORQUE : 0|10@0+ (1,0) [-256|256] "" EON
//  SG_ OUTPUT_DISABLED_INVERT : 15|1@0+ (1,0) [0|1] "not sure if its invert or some LDW" EON
//  SG_ UNK_3BIT_1 : 10|3@0+ (1,0) [0|7] "" EON
//  SG_ OUTPUT_DISABLED : 14|1@0+ (1,0) [0|1] "" EON
//  SG_ COUNTER : 21|2@0+ (1,0) [0|3] "" EON
//  SG_ CHECKSUM : 19|4@0+ (1,0) [0|15] "" EON

//removed
// SG_ CONFIG_VALID : 7|1@0+ (1,0) [0|1] "" EON   << not used

void buildSteerMotorTorqueCanMsg(){ //TODO: add to decclaration
	//outputSerial.print("\nSendingSteer TOrque Can MSg");
	CAN_message_t msg; // move this to a global to save the assignment of id and len
	msg.id = 427;
	msg.len = 3;
	msg.buf[0] = (EPStoLKASBuffer[2] << 4 ) & B1000000;  //1 LSB bit of bigSteerTorque 
	msg.buf[0] |= EPStoLKASBuffer[3] & B01111111 ; // all of SmallSteerTorque
	msg.buf[1] = ( EPStoLKASBuffer[2] >> 4 ) & B00000011; // 2 MSB of bigSteerTorque
	msg.buf[1] |= ( EPStoLKASBuffer[1] << 2 ) & B10000000;  // this is output_disabled_inverted
	msg.buf[1] |= ( EPStoLKASBuffer[2] << 2 )& B00011100; //UNK_3bit_1
	msg.buf[1] |=  EPStoLKASBuffer[2] & B01000000; //output_disabled
	
	msg.buf[2] = (OPCanCounter << 4 ); // put in the counter
	msg.buf[2] |= honda_compute_checksum(&msg.buf[0],3,(unsigned int)msg.id);
	// FCAN.write(msg);
	can.transmit(msg.id,msg.buf, msg.len);
}

///// the only thing in this DBC that should be used is steeor_troque_sensor
// BO_ 399 STEER_STATUS: 3 EPS
//  SG_ COUNTER : 21|2@0+ (1,0) [0|3] "" EON
//  SG_ CHECKSUM : 19|4@0+ (1,0) [0|3] "" EON
//  SG_ STEER_TORQUE_SENSOR : 0|9@1- (1,0) [-256|256] "" XXX


// T T T T  T T T T
// S S S S  E L 0 T
// 0 0 C C  H H H H
//removed
//  SG_ STEER_STATUS : 39|4@0+ (1,0) [0|15] "" EON
//  SG_ STEER_CONTROL_ACTIVE : 35|1@0+ (1,0) [0|1] "" EON
//  SG_ STEER_CONFIG_INDEX : 43|4@0+ (1,0) [0|15] "" EON
//  SG_ STEER_ANGLE_RATE : 23|16@0- (-0.1,0) [-31000|31000] "deg/s" EON << TODO: check if OP uses this or the other STEER_ANGLE_RATE .. this one will not work

void buildSteerStatusCanMsg(){ //TODO: add to decclaration
	
	// outputSerial.print("\nsending Steer Status Cna MSg");
	CAN_message_t msg; // move this to a global so you dont have to re assign the id and len
	msg.id = 399;
	msg.len = 3;
	msg.buf[0] = EPStoLKASBuffer[0] << 5;   // 3 LSB of BigSteerTorque (4bit)
	msg.buf[0] |= EPStoLKASBuffer[1] & B00011111; // all of smallSteerTorque
	msg.buf[0] = ~msg.buf[0]; // invert the whole message to make negative positive, positive negative.  OP wants left positive (why??)
	msg.buf[1] =  ( ~(  EPStoLKASBuffer[0] >>3 ) )   & B00000001; // 1st MSB of bigSteerTorque (4bit) ... added NOT (~) to invert the sign


	//add other data from Teensy so OP can record it
	msg.buf[1] |= LkasFromCanStatus << 4; 
	msg.buf[1] |= OPLkasActive << 2;
	msg.buf[1] |= LkasFromCanFatalError << 3;
	
	msg.buf[2] = (OPCanCounter << 4 ); // put in the counter
	msg.buf[2] |= honda_compute_checksum(&msg.buf[0],3,(unsigned int) msg.id);
	// FCAN.write(msg);
	can.transmit(msg.id,msg.buf, msg.len);
}





void handleLkasFromCanV3(){
// 	BO_ 228 STEERING_CONTROL: 5 ADAS
//  SG_ STEER_TORQUE : 7|16@0- (1,0) [-3840|3840] "" EPS
//  SG_ STEER_TORQUE_REQUEST : 23|1@0+ (1,0) [0|1] "" EPS
//  SG_ SET_ME_X00 : 31|8@0+ (1,0) [0|0] "" EPS
//  SG_ COUNTER : 37|2@0+ (1,0) [0|3] "" EPS
//  SG_ CHECKSUM : 35|4@0+ (1,0) [0|3] "" EPS

	if(canMsg.id != 228) return;

#ifdef DEBUG_PRINT_OPtoCAN_INPUT
	outputSerial.print("\nCANmsg rcvd id: ");
	outputSerial.print(canMsg.id,DEC);
	outputSerial.print(":");
	for(uint8_t bb = 0; bb < canMsg.len; bb++){
		printuint_t(canMsg.buf[bb]);
	}
#endif 

	if((canMsg.buf[2] >> 7) == 1 ){ // if STEER REQUEST (aka LKAS enabled)
		OPLkasActive = true;
	} else {
		OPLkasActive = false;
	}

	uint8_t lclBigSteer = 0;
	uint8_t lclLittleSteer = 0;
	
	lclBigSteer = ( canMsg.buf[0] >> 4 ) & B00001000;
	lclBigSteer |= ( canMsg.buf[1] >> 5 ) & B00000111;
	
	lclLittleSteer = canMsg.buf[1] & B00011111 ;
	
		// TODO: verify counter is working
	uint8_t lclCounter = canMsg.buf[4] >> 4;
	bool counterVerified = false;  // need global counter   and counter error

	if(LkasFromCanCounter != lclCounter) LkasFromCanCounterErrorCount++;
	else LkasFromCanCounterErrorCount = 0;
	
	if(LkasFromCanCounter < 3) counterVerified = true;


	// TODO: verify checksum
	bool checksumVerified = false;

	if(honda_compute_checksum((uint8_t*) &canMsg.buf[0],5, 228U) == (canMsg.buf[5] & B00001111 )) LkasFromCanChecksumErrorCount = 0;
	else LkasFromCanChecksumErrorCount++;
	
	if(LkasFromCanCounterErrorCount < 3 ) checksumVerified = true;
	else checksumVerified = false;


	// TODO: Fix this, hard coded to true for testing
	counterVerified = true;
	checksumVerified = true;

	//canbus data time is checked in the handleLkastoEPS function, if no data has been received within 50ms . LKAS is not allowed to be active

	// set big/small steer in varible and that LKAS is on
	// so when its time to send a LKAS message, it just reads the data, make the checksum and send it
	if(counterVerified && checksumVerified){
		// createKLinMessageWBigSteerAndLittleSteer(lclBigSteer,lclLittleSteer);
		
		OPBigSteer = lclBigSteer;
		OPLittleSteer = lclLittleSteer;
		OPApply_steer = (lclBigSteer & B00000111) << 5;
		OPApply_steer |= lclLittleSteer;
		if((lclBigSteer >> 3) == 1) OPApply_steer |= 0xFF00; 
		
	} else{
		OPLkasActive = false;
		// TODO: send/set/notify something to show there was an error... 
	}
	OPSteeringControlMessageStatusPending = true;
	OPTimeLastCANRecieved = millis();
}


#endif