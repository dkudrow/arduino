//===-- ps2keyboard.cpp - PS/2 Keyboard Host and Device Emulation ----------===/
//
// Written by Daniel Kudrow (dkudrow@cs.ucsb.edu)
// August 2013
//
// Library to emulate device and hosts of PS/2 protocol
//
// TODO:
// check data/clock state before acting
// handle host inhibition
// listen for host requests to send
// handle host interrupt
//
//===-----------------------------------------------------------------------===/

#include "ps2emu.h"

int PS2Emu::hostRead(unsigned char *ret) {
  unsigned char data = 0x00;
  unsigned char p = 0x01;

  // discard the start bit
  while (digitalRead(clk) == HIGH);
  while (digitalRead(clk) == LOW);  
  
  // read each data bit
  for (int i=0; i<8; i++) {
    while (digitalRead(clk) == HIGH);
    if (digitalRead(dat) == HIGH) {
      data = data | (1 << i);
      p = p ^ 1;
    }
    while (digitalRead(clk) == LOW);
  }
  
  // read the parity bit
  while (digitalRead(clk) == HIGH);
  if (digitalRead(dat) != p) {
    return PARITY;
  }
  while (digitalRead(clk) == LOW);
  
  // discard the stop bit
  while (digitalRead(clk) == HIGH);
  while (digitalRead(clk) == LOW);
  
  *ret = data;
  return SUCCESS;
}

int PS2Emu::devRead(unsigned char *ret) {
  unsigned char data = 0x00;
  unsigned char p = 0x01;
  
  // wait for the host to release the clock
  while (digitalRead(dat) == HIGH);
  while (digitalRead(clk) == LOW);
  
  // read start bit
  delayMicroseconds(cell/2);
  setLow(clk);
  delayMicroseconds(cell);
  setHigh(clk);
  delayMicroseconds(cell/2);

  // read data bits
  for (int i=0; i<8; i++) {
    if (digitalRead(dat) == HIGH) {
      data = data | (1 << i);
      p = p ^ 1;
    }
    delayMicroseconds(cell/2);
    setLow(clk);	
    delayMicroseconds(cell);
    setHigh(clk);
    delayMicroseconds(cell/2);
  }

  // read parity bit
  if (digitalRead(dat) != p) {
    return PARITY;
  }
  delayMicroseconds(cell/2);
  setLow(clk);	
  delayMicroseconds(cell);
  setHigh(clk);
  delayMicroseconds(cell/2);
  
  // send 'ack' bit
  setLow(dat);
  delayMicroseconds(cell/2);
  setLow(clk);	
  delayMicroseconds(cell);
  setHigh(clk);
  setHigh(dat);

	// the Arduino has large pull-up resistors so give the data line
	// some time to release
	delayMicroseconds(100);

  *ret = data;
  return SUCCESS;
}

int PS2Emu::hostWrite(unsigned char data) {
  unsigned char p = 0x01;

  // inhibit device transmission
  setLow(clk);
  delayMicroseconds(100);
  // send request to communicate with device
  setLow(dat);
  //delayMicroseconds(cell/2); // could be wrong
  setHigh(clk);

  // send data
  for (int i=0; i<8 ;i++) {
    while (digitalRead(clk) == HIGH);
    if (data & (1 << i)) {
      setHigh(dat);
      p = p ^ 1;
    } else {
      setLow(dat);
    }
    while (digitalRead(clk) == LOW);
  }
  
  // send parity bit
  while (digitalRead(clk) == HIGH);
  if (p) {
    setHigh(dat);
  } else {
    setLow(dat);
  }
  while (digitalRead(clk) == LOW);

  // send stop bit
  while (digitalRead(clk) == HIGH);
  setHigh(dat);
  while (digitalRead(clk) == LOW);

  // eat the 'ack' bit
  while (digitalRead(clk) == HIGH);
  while (digitalRead(clk) == LOW);

  return SUCCESS;
}

int PS2Emu::devWrite(unsigned char data) {
  unsigned char p = 0x01;;
  int clk_start, clk_stop;

  // clock must be high for 50 microseconds before transmission
  /*
  if (checkState(clk, LOW, 50))
      return INHIBIT;
      */

  // set start bit
  setLow(dat);
  delayMicroseconds(cell/2);
  /*
  if (!checkState(clk, LOW, 100))
    return INHIBIT;
      */
  setLow(clk);
  delayMicroseconds(cell);
  setHigh(clk);
  delayMicroseconds(cell/2);

  // set data bits
  for (int i=0; i<8; i++) {
    if (data & (1 << i)) {
      setHigh(dat);
      p = p ^ 1;
    } else {
      setLow(dat);
    }
    delayMicroseconds(cell/2);
  /*
    if (!checkState(clk, LOW, 100))
      return INHIBIT;
      */
    setLow(clk);
    delayMicroseconds(cell);
    setHigh(clk);
    delayMicroseconds(cell/2);
  }

  // set parity bit
  if (p) {
    setHigh(dat);
  } else {
    setLow(dat);
  }
  delayMicroseconds(cell/2);
  /*
  if (!checkState(clk, LOW, 100))
    return INHIBIT;
      */
  setLow(clk);
  delayMicroseconds(cell);
  setHigh(clk);
  delayMicroseconds(cell/2);

  // stet stop bit
  setHigh(dat);
  delayMicroseconds(cell/2);
  setLow(clk);
  delayMicroseconds(cell);
  setHigh(clk);
  delayMicroseconds(cell/2);

  return SUCCESS;
}

// passively listen to communication between a device and a host
// I wouldn't have to write this if I had an o-scope...
int PS2Emu::listen(unsigned char *ret) {
	// wait for a frame
	while (digitalRead(dat) == HIGH);	
	// check to see who's sending
	if (digitalRead(clk) == HIGH) { // device-to-host
		hostRead(ret);
		return 0;
	} else { // host-to-device
		devRead(ret);
		return 1;
	}
	return -1;
}

// check that pin does not enter state for t microseconds
bool PS2Emu::checkState(int pin, int state, int t) {
  int clk_start, clk_stop;

  clk_start = clk_stop = micros();
  while (clk_stop - clk_start < t) {
    if (digitalRead(pin) == state)
      return true;
  }

  return false;
}

// You may have to modify this to suit your needs
bool PS2Emu::handshake() {
	unsigned char data = 0x00;

	// tell host we are ready to connect
	devWrite(0xAA);

	// wait for response
	while (checkState(dat, LOW, 5000)) {
		devRead(&data);
		devWrite(0xFA);
		switch(data) {
			case 0x00: // second bit of 0xED or 0xF3
				break;
			case 0xED: // set/reset LEDs
				break;
			case 0xF2: // ID
				devWrite(0xAB);
				devWrite(0x83);
				break;
			case 0xF3: // set/reset typematic delay
				break;
			case 0xF4: // keyboard is enabled, break loop
				return true;
		}
	}
	return false;
}
