#include "x-touch.h"
#include <chrono>
#include <thread>
#include <XController.h>
#include <assert.h>
#include <delayed.h>
#include <interface.h>

// Global pointer to the XTouch object
// It is preferable to use a global pointer to the XTouch object 
// as the XTouch methods for updating segments/dials/faders is used by all the individual physical display objects
// and passing callbacks to each of these objects would either be beuracratic or convoluted.
// This is a simple way to ensure that the XTouch object is available to all the physical display objects
// The "proper" way to do this would be to use message passing/mailbox, but that is only necessary if our threading model
// becomes more complex.
XTouch *g_xtouch;
DelayedExecuter *g_delayedThreadScheduler;
InterfaceManager *g_interfaceManager;

int main(int, char**) {
   g_xtouch = new XTouch();
   g_delayedThreadScheduler = new DelayedExecuter();
   g_interfaceManager = new InterfaceManager(g_xtouch);
   
   XTouchController controller;
   while(true) {
      std::this_thread::sleep_for(std::chrono::hours(24));
   }
   assert(false && "Should never reach here");
   return 0;
}