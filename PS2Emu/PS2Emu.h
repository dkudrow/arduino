//===-- ps2keyboard.h - PS/2 Keyboard Host and Device Emulation ----------===/
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


#ifndef ps2emu_h
#define ps2emu_h

#include "Arduino.h"

#define SUCCESS 0
#define PARITY -1
#define INHIBIT -2

class PS2Emu {
  private:
    int clk; // clock pin
    int dat; // data pin
    int cell; // one half of clock cycle time
    // logical high is high-impedance
    void setHigh(int pin) { pinMode(pin, INPUT); digitalWrite(pin, HIGH); }
    // logical low is low-impedance
    void setLow(int pin) { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }
    bool checkState(int pin, int state, int t);

  public:
    // construct and initialize
    PS2Emu(int c, int d, int t=40) : clk(c), dat(d), cell(t) {
	    setHigh(clk);
	    setHigh(dat);;
    }
    // read as host (from device)
    int hostRead(unsigned char *data);
    // read as device (from host)
    int devRead(unsigned char *data);
    // write as host (to device)
    int hostWrite(unsigned char data);
    // write as device (to host)
    int devWrite(unsigned char data);
		// listen to communications on a PS/2 line
		int listen(unsigned char *data);
		// initiate communication with a PC
		// note: this is a pretty hacky method, do not use it when interfacing with
		// pacemakers and critical intrastructure
		bool handshake();
};

#endif
