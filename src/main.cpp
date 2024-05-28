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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <thread>
#include <chrono>
#include <assert.h>
#include <vector>
#include <map>
#include <string>
#include <queue>
#include <type_traits>
#include <memory>
#include <atomic>

#include "x-touch.h"

#define BUFSIZE 1508

typedef struct {
    int sockfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
} socketinfo_t;

typedef struct {
    xt_ScribblePad_t pad;
    int mainlevel;
    int mute;
    int trimlevel;
    int pan;
    int solo;
    int rec;
    int mode;
} channelinfo_t;

{
    int channel;
    if (value>0) {
        printf("Dial %d clockwise by %d clicks\n", dial, value);
    } else {
        printf("Dial %d anti-clockwise by %d clicks\n", dial, 0-value);
    }
    if ((dial>15)&&(dial<24)) {
        channel=dial-16;
        if (value>0) {
            switch (channels[page*8+channel].mode) {
                case 0: // Pan
                        if (channels[page*8+channel].pan<6)  channels[page*8+channel].pan++;
                        break;
                case 1: // Trim
                        if (channels[page*8+channel].trimlevel<13)  channels[page*8+channel].trimlevel++;
                        break;
                case 2: // Colour
                        if (channels[page*8+channel].pad.Colour<WHITE) channels[page*8+channel].pad.Colour=(xt_colours_t)(((int)(channels[page*8+channel].pad.Colour))+1);
                        break;
            }
        } else {
            switch (channels[page*8+channel].mode) {
                case 0: // Pan
                        if (channels[page*8+channel].pan>-6)  channels[page*8+channel].pan--;
                        break;
                case 1: // Trim
                        if (channels[page*8+channel].trimlevel>0)  channels[page*8+channel].trimlevel--;
                        break;
                case 2: // Colour
                        if (channels[page*8+channel].pad.Colour>BLACK) channels[page*8+channel].pad.Colour=(xt_colours_t)(((int)(channels[page*8+channel].pad.Colour))-1);
                        break;
            }            
        }
        RenderDial((XTouch*)data, channel);
    }
    if (dial==60) {
        if (value>0) {
            if (selected<63) selected++;
        } else {
            if (selected>0) selected--;            
        }
        RenderPageAndSelected((XTouch*)data);
        RenderSelectedButton((XTouch*)data);
    }
}
