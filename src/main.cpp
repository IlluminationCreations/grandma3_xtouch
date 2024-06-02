/* ------------------------------------------------------------------------------
   This is a sample application to demonstrate how to use the x-touch library.
   It makes the x-touch behave in a way similar to a simple 64 channel desk.
   Note: This is an interface demonstration only - no audio processing is done!
   -----------------------------------------------------------------------------*/

/*
MIT License

Copyright (c) 2020 Martin Whitaker

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/* The X-Touch must have firmware version 1.15 in order to use Xctl mode
   To upgrade using Linux download the firmware from
   http://downloads.music-group.com/software/behringer/X-TOUCH/X-TOUCH_Firmware_V1.15.zip
   Connect the X-Touch via USB to the PC and run the following command:
   amidi -p hw:1,0,0 -s X-TOUCH_sysex_update_1-15_1-03.syx -i 100
*/

/* To configure the X-Touch for XCtl use the following procedure:
   - Hold select of CH1 down whilst the X-Touch is turned on
   - Set the mode to Xctl
   - Set the Ifc to Network
   - Set the Slv IP to the IP address of your PC
   - Set the DHCP on (or set a static IP on the X-Touch as desired)
   - Press Ch1 select to exit config mode
*/

#include "x-touch.h"
#include <chrono>
#include <thread>
#include <XController.h>
#include <assert.h>
#include <delayed.h>

// Global pointer to the XTouch object
// It is preferable to use a global pointer to the XTouch object 
// as the XTouch methods for updating segments/dials/faders is used by all the individual physical display objects
// and passing callbacks to each of these objects would either be beuracratic or convoluted.
// This is a simple way to ensure that the XTouch object is available to all the physical display objects
// The "proper" way to do this would be to use message passing/mailbox, but that is only necessary if our threading model
// becomes more complex.
XTouch *g_xtouch;
DelayedExecuter *g_delayedThreadScheduler;

int main(int, char**) {
   g_xtouch = new XTouch();
   XTouchController controller;
   while(true) {
      std::this_thread::sleep_for(std::chrono::hours(24));
   }
   assert(false && "Should never reach here");
   return 0;
}