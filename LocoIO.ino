/*
 * LocoIO - SV-Editor for LocoIO
 
used I²C-Addresses:
  - 0x20  LCD-Panel ODER:
  
  - 0x78  OLED-Panel mit
  - 0x23  Button am OLED-Panel
  
  - 0x39  Keypad 4x4
  
discrete In/Outs used for functionalities:
  -  0     (used  by USB)
  -  1     (used  by USB)
  -  2     [INT]
  -  3 Out (usable) D5 gn
  -  4 Out [CS]   (used  by ETH-Shield for SD-Card-select, Remark: memory using SD = 4862 Flash, 791 RAM))
       In  (usable) S2
  -  5 In  used   S3, Ctrl-Taster
  -  6 Out used   by HeartBeat
  -  7 Out used   by LocoNet [TxD]
  -  8 In  used   by LocoNet [RxD]
  -  9 Out used   LN-Activity (LocoNet [CTS])
  - 10 Out [SS]   (used  by ETH-Shield for W1500-select) 
       Out (usable) D6 rt
  - 11     [MOSI] (used  by ETH-Shield) 
  - 12     [MISO] (used  by ETH-Shield) 
  - 13     [SCK]  (used  by ETH-Shield) 
  - 14
  - 15
  - 16
  - 17
  - 18     (used by I²C: SDA)
  - 19     (used by I²C: SCL)

 *************************************************** 
 *  Copyright (c) 2018 Michael Zimmermann <http://www.kruemelsoft.privat.t-online.de>
 *  All rights reserved.
 *
 *  LICENSE
 *  -------
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************
 */

//=== global stuff =======================================
//#define DEBUG 1   // enables Outputs (debugging informations) to Serial monitor
                  // note: activating SerialMonitor in Arduino-IDE
                  //       will create a reset in software on board!

//#define DEBUG_CV 1 // enables CV-Output to serial port during program start (saves 180Bytes of code :-)
//#define DEBUG_MEM 1 // enables memory status on serial port (saves 350Bytes of code :-)

//#define USE_SD_CARD_ON_ETH_BOARD 1			// enables pin 4 for cardselect of SD-card on Ethernet-Board
//#define ETHERNET_BOARD 1								// needs ~13.8k of code, ~320Bytes of RAM
//#define ETHERNET_WITH_LOCOIO 1
//#define ETHERNET_PAGE_SERVER 1

//#define FAST_CLOCK_LOCAL 1  //use local ports for slave clock if no I2C-clock is found.

//#define TELEGRAM_FROM_SERIAL 1  // enables receiving telegrams from SerialMonitor
                                // instead from LocoNet-Port (which is inactive then)

//========================================================

//#define LCD     // used in GlobalOutPrint.ino, LCDPanel.ino
#define OLED    // used in OLEDPanel.ino

#if defined LCD
  #if defined OLED
    #error LCD and OLED defined
  #endif
#endif
#if !defined LCD
  #if !defined OLED
    #error Neither LCD nor OLED defined
  #endif
#endif

#include "CV.h"

#define ENABLE_LN             (1)
#define ENABLE_LN_E5          (1)
#define ENABLE_LN_FC_MODUL    (0)
#define ENABLE_LN_FC_INTERN   (0)
#define ENABLE_LN_FC_INVERT   (0)

#define ENABLE_KEYPAD         (1)

#define UNREFERENCED_PARAMETER(P) { (P) = (P); }

#define MANUFACTURER_ID  13   // NMRA: DIY
#define DEVELOPER_ID  58      // NMRA: my ID, should be > 27 (1 = FREMO, see https://groups.io/g/LocoNet-Hackers/files/LocoNet%20Hackers%20DeveloperId%20List_v27.html)

//========================================================

#include <LocoNet.h>  // requested for notifySwitchRequest, notifySwitchReport, notifySwitchOutputsReport, notifySensor

#include <HeartBeat.h>
HeartBeat oHeartbeat;

//========================================================
void setup()
{
#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
  // initialize serial and wait for port to open:
  Serial.begin(57600);
#endif

  ReadCVsFromEEPROM();
  
  CheckAndInitDisplayPanel();

#if defined ENABLE_KEYPAD
  CheckAndInitKeypad();
#endif

  InitLocoNet();
  
#if defined ETHERNET_BOARD
  InitEthernet();
#endif
}

void loop()
{
  // light the Heartbeat LED
  oHeartbeat.beat();
  // generate blinken
  Blinken();

  //=== do LCD handling ==============
  // can be connected every time
  // panel necessary for edit SV's and setup CV's (or some status informations):
#if defined ENABLE_KEYPAD
  HandleKeypad();
#endif
  HandleDisplayPanel();

  //=== do LocoNet handling ==========
  HandleLocoNetMessages();

#if defined ETHERNET_BOARD
  //=== react on Ethernet requests ===
  HandleEthernetRequests();
#endif

#if defined DEBUG
  #if defined DEBUG_MEM
    ViewFreeMemory();  // shows memory usage
    ShowTimeDiff();    // shows time for 1 cycle
  #endif
#endif
}

//=== will be called from LocoNet-Class
// Address: Switch Address.
// Output: Value 0 for Coil Off, anything else for Coil On
// Direction: Value 0 for Closed/GREEN, anything else for Thrown/RED
// state: Value 0 for no input, anything else for activated
// Sensor: Value 0 for 'Aux'/'thrown' anything else for 'switch'/'closed'
void notifySwitchRequest(uint16_t Address, uint8_t Output, uint8_t Direction)
{ // OPC_SW_REQ (B0) received:
  workOnNotify(Address, Output, Direction);
}

void notifySwitchReport(uint16_t Address, uint8_t Output, uint8_t Direction)
{ // OPC_SW_REP (B1) received, (LnPacket->data[2] & 0x40) == 0x40:
  workOnNotify(Address, Output, Direction);
}

void notifySwitchOutputsReport(uint16_t Address, uint8_t ClosedOutput, uint8_t ThrownOutput)
{ // OPC_SW_REP (B1) received, (LnPacket->data[2] & 0x40) == 0x00:
  workOnNotify(Address, ThrownOutput, ClosedOutput);
}

void notifySensor(uint16_t Address, uint8_t State)
{ // OPC_INPUT_REP (B2) received:
  workOnNotify(Address, 0x10, State);
}
