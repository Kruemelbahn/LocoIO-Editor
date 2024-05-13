//=== LocoNetDispalyMsg.ino for LocoIO usable with OLED ===
#if defined LCD
  // current LCD size: 16 chars in 2 lines
  #define COUNT_OF_CHARS 16
  #define COUNT_OF_LINES 2
#endif

#if defined OLED
  // current OLED size: 128 x 64 px == 21 chars in 8 lines
  // we use defines instead of '#include <OLEDPanel.h>' => this gives more flexibility e.g. for TFT
  #define COUNT_OF_CHARS 21
  #define COUNT_OF_LINES 8
#endif

#define OPC_LOCO_F912       0xA3

enum { None = 0, SerialPass = 1, LocoPass = 2, SerialError = 5, LocoError = 6 };

String displayStrings[COUNT_OF_LINES];
short displayLineIndex = 0;

boolean b_NewMonitorTelegram = false;
boolean HasNewMonitorTelegram() { return b_NewMonitorTelegram; }

void clearDisplayStrings()
{
  for (int i = 0; i < COUNT_OF_LINES; i++)
    displayStrings[i] = "";
}

String getByteAsHEXString(const byte oneByte)
{
  String byteString(String(oneByte, HEX));
  byteString.toUpperCase();
  if (byteString.length() < 2)
    byteString = "0" + byteString;
  return byteString;
}

void addAdressAndStateFromB0B1B2(const byte bytes[])
{
  //--- following similar to: ...\libraries\LocoNet\LocoNet.cpp [uint8_t LocoNetClass::processSwitchSensorMessage(lnMsg* LnPacket)]
	uint16_t Address(bytes[1] | ((bytes[2] & 0x0F) << 7));
	if (bytes[0] != OPC_INPUT_REP)
		Address++;
  else
  {
  	Address <<= 1;
		Address += (bytes[2] & OPC_INPUT_REP_SW) ? 2 : 1;
  }
  //---

  displayStrings[displayLineIndex] += getByteAsHEXString((uint8_t)(Address >> 8));
  displayStrings[displayLineIndex] += getByteAsHEXString((uint8_t)(Address & 0xFF));
  displayStrings[displayLineIndex] += ' ';
  displayStrings[displayLineIndex] += getByteAsHEXString(bytes[2] & 0xF0);
}

void addByte1AndByte2(const byte bytes[])
{
  displayStrings[displayLineIndex] += getByteAsHEXString(bytes[1]);
  displayStrings[displayLineIndex] += ' ';
  displayStrings[displayLineIndex] += getByteAsHEXString(bytes[2]);
}

byte getMessageLength(const byte bytes[], byte byteCount)
{
  byte messageLength(0);
  if (byteCount)
  {
    switch (bytes[0] & 0xE0)
    {
      case 0xE0: //N byte message
        if (byteCount >= 2 ) //length byte available
          messageLength = bytes[1];
        break;
      case 0xC0: //6 byte message
        messageLength = 6;
        break;
      case 0xA0: //4 byte message
        messageLength = 4;
        break;
      case 0x80: //2 byte message
        messageLength = 2;
        break; 
    } // switch (bytes[0] & 0xE0)
  } // if (byteCount) 
  return messageLength;
} // byte getMessageLength(const byte bytes[], byte byteCount)

void updateDisplayStrings(const uint8_t messageState, const byte bytes[], const byte byteCount)
{
  incrementDisplayLine();

  switch (messageState)
  {
    case SerialPass:
      displayStrings[displayLineIndex]  = F(" > :");
      break;
    case SerialError:
      displayStrings[displayLineIndex]  = F(" >X:");
      break;
    case LocoPass:
      displayStrings[displayLineIndex]  = F(" < :");
      break;
    case LocoError:
      displayStrings[displayLineIndex]  = F("X< :");
      break;
    default:
      displayStrings[displayLineIndex]  = "";
  } // switch (messageState)

  // check for decodeable messages:
  switch(bytes[0])
  {
    case OPC_BUSY: // 0x81
      displayStrings[displayLineIndex] += F("BUSY"); 
      break;
    case OPC_GPOFF: // 0x82
      displayStrings[displayLineIndex] += F("GPOFF"); 
      break;
    case OPC_GPON: // 0x83 
      displayStrings[displayLineIndex] += F("GPON"); 
      break;
    case OPC_IDLE: // 0x85 
      displayStrings[displayLineIndex] += F("IDLE"); 
      break;
    case OPC_SW_REQ: // 0xB0 
      displayStrings[displayLineIndex] += F("SW_REQ ");
      addAdressAndStateFromB0B1B2(bytes);
      break;
    case OPC_SW_REP: // 0xB1 
      displayStrings[displayLineIndex] += F("SW_REP ");
      addAdressAndStateFromB0B1B2(bytes);
      break;
    case OPC_INPUT_REP: // 0xB2 
      displayStrings[displayLineIndex] += F("INPUT_REP ");
      addAdressAndStateFromB0B1B2(bytes);
      break;
    case OPC_LONG_ACK: // 0xB4 
      displayStrings[displayLineIndex] += F("LACK ");
      addByte1AndByte2(bytes);
      break;
    case OPC_MOVE_SLOTS: // 0xBA 
      displayStrings[displayLineIndex] += F("MOVE_SLOT ");
      addByte1AndByte2(bytes);
      break;
    case OPC_RQ_SL_DATA: // 0xBB 
      displayStrings[displayLineIndex] += F("RQ_SL_DATA ");
      displayStrings[displayLineIndex] += getByteAsHEXString(bytes[1]);
      break;
    case OPC_SW_STATE: // 0xBC
      displayStrings[displayLineIndex] += F("SW_STATE ");
      addAdressAndStateFromB0B1B2(bytes);
    case OPC_LOCO_ADR: // 0xBF
      displayStrings[displayLineIndex] += F("LOCO_ADR ");
      displayStrings[displayLineIndex] += getByteAsHEXString(bytes[1]);
      displayStrings[displayLineIndex] += getByteAsHEXString(bytes[2]);
      break;
    case OPC_LOCO_SPD:  // 0xA0
    case OPC_LOCO_DIRF: // 0xA1
    case OPC_LOCO_SND:  // 0xA2
    case OPC_LOCO_F912: // 0xA3
      if(bytes[0] == OPC_LOCO_SPD)
        displayStrings[displayLineIndex] += F("LOCO_SPD ");
        else  if(bytes[0] == OPC_LOCO_DIRF)
                displayStrings[displayLineIndex] += F("LOCO_DIRF ");
              else  if(bytes[0] == OPC_LOCO_SND)
                      displayStrings[displayLineIndex] += F("LOCO_SND ");
                    else
                      displayStrings[displayLineIndex] += F("LOCO_F912 ");
      addByte1AndByte2(bytes);
      break;
    default:
      for (int i = 0; i < byteCount; i++)
      {
        if ( i && !(i % 8) )
        { // go to next line after 8 bytes
          incrementDisplayLine();
          displayStrings[displayLineIndex] = messageState == None ? F("  ") : F("    ");
        }
        displayStrings[displayLineIndex] += getByteAsHEXString(bytes[i]);
      } // for ( int i = 0; i < byteCount; i++)
      break;
  } // switch(bytes[0])
} // void updateDisplayStrings(MessageStates messageState, const byte bytes[], byte byteCount)

bool testCheckSum(const byte bytes[], const byte byteCount)
{
  byte check(0);

  for (int i = 0; i < byteCount; i++)
    check ^= bytes[i];

  return check == 0xFF;
}

void incrementDisplayLine()
{
  displayLineIndex++;
  if (displayLineIndex >= COUNT_OF_LINES)
  {
    if (displayStrings[COUNT_OF_LINES].length())
    {
      for (int i = 1; i < COUNT_OF_LINES; i++)
        displayStrings[i - 1] = displayStrings[i];
    }
    displayLineIndex = COUNT_OF_LINES - 1;
  }
} // void incrementDisplayLine()

void monitorLocoNetMessage(volatile lnMsg* lnPacket, const uint8_t messageState)
{
  if (isLNMonitorActive() && lnPacket)
  {
    // Get the length of the received packet
    const byte Length(getLnMsgSize(lnPacket));
    if (testCheckSum(lnPacket->data, Length))
      updateDisplayStrings(messageState, lnPacket->data, Length);
    else
      updateDisplayStrings(messageState | 0x04, lnPacket->data, Length);
    b_NewMonitorTelegram = true;
  }
} // void monitorLocoNetMessage(volatile lnMsg* lnPacket)

String getDisplayString(uint8_t iLine)
{
  if(iLine >= COUNT_OF_LINES)
    return "";
  if(iLine == (COUNT_OF_LINES - 1) )
    b_NewMonitorTelegram = false;  
  return displayStrings[iLine];
} // String getDisplayString(uint8_t iLine)
