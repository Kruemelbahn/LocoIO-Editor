//=== LocoNet for LocoIO ===
#include <LocoNet.h>
#include <LocoNetKS.h>

//=== declaration of var's =======================================
static  lnMsg   *LnPacket;
static  LnBuf   LnTxBuffer;

uint8_t ui8_WaitForTelegram = 0;  /* indicates: we're waiting for a telegram
                                  as long as this value is not zero,
                                  we can't send any other telegram
                                  0x01 : send first sv
                                  0x02 : send second sv
                                  0x03 : send third sv
                                  0x08 : read one port
                                  0x40 : read B0/B1/B2-telegrams
                                  0x80 : read all addresses
                                  */
unsigned long ul_WaitStartForTelegram = 0;

/*
*  https://www.digitrax.com/static/apps/cms/media/documents/loconet/loconetpersonaledition.pdf
*  https://wiki.rocrail.net/doku.php?id=loconet:ln-pe-de
*  http://embeddedloconet.sourceforge.net/SV_Programming_Messages_v13_PE.pdf
*  remark: this software didnot follow '2.2.6) Standard SV/EEPROM Locations'
*/
// default OPC's are defined in 'utility\ln_opc.h'
// A3 is already used  and defined as OPC_LOCO_F912
// A8 is already used by FRED  and defined as OPC_DEBUG
// A8 is already used by FREDI and defined as OPC_FRED_BUTTON
//#define OPC_FRED_BUTTON     0xA8
// AF is already used by FREDI and defined as OPC_SELFTEST resp. as OPC_FRED_ADC
// ED is already used  and defined as OPC_IMM_PACKET
// EE is already used  and defined as OPC_IMM_PACKET_2
#define SV2_Format_1	0x01
#define SV2_Format_2	0x02

#define PIN_TX   7
#define PIN_RX   8

#define PIN_LN_STATUS_LED 9

//=== keep informations for telegram-sequences ===================
uint8_t ui8_ModulAddr;
uint8_t ui8_ModulSubAddr;
uint8_t ui8_ModulSV;
uint8_t ui8_Value1, ui8_Value2, ui8_Value3;

boolean b_ReadAllModulAddressesFinished = false;
boolean b_NewTelegramAvailable = false;
uint8_t ui8_MaxModulDataRead = 0;
#define MAX_MODULDATA 128
uint8_t GetModulMaxData() { return MAX_MODULDATA; }
struct ModulData
{
  union
  { 
    struct
    {
      uint8_t ui8_Address;
      uint8_t ui8_SubAddress;
    } modulAddress;
    uint16_t ui16_ioAddress;
  };  // union hier immer 2-Byte
  union
  {
    uint8_t ui8_Version;
    uint8_t ui8_State;  // ...S ..XX => S=State (on or off), XX=Telegramcode BX
  }; // union hier immer 1-Byte (boolean ist auch 1-Byte)
} st_ModulData[MAX_MODULDATA];  // ein Datensatz ist immer 3-Byte

//=== functions for receiving telegrams from SerialMonitor =======
// Eingabe Byteweise
// jedes Byte besteht aus zwei Hex-Zahlen
// Groß/Klein spielt keine Rolle
// Leerzeichen zwischen zwei Bytes erforderlich
// Prüfsumme wird am Ende automatisch ermittelt und hinzugefügt
#if defined TELEGRAM_FROM_SERIAL
static const int MAX_LEN_LNBUF = 64;
uint8_t ui8_PointToBuffer = 0;

uint8_t ui8a_receiveBuffer[MAX_LEN_LNBUF];
uint8_t ui8a_resultBuffer[MAX_LEN_LNBUF];

void ClearReceiveBuffer()
{
  ui8_PointToBuffer = 0;
  for(uint8_t i = 0; i < MAX_LEN_LNBUF; i++)
    ui8a_receiveBuffer[i] = 0;
}

uint8_t Adjust(uint8_t ui8_In)
{
  uint8_t i = ui8_In;
  if((i >= 48) && (i <= 57))
    i -= 48;
  else
    if((i >= 65) && (i <= 70))
      i -= 55;
    else
      if((i >= 97) && (i <= 102))
        i -= 87;
      else
        i = 0;
  return i;
}

uint8_t AnalyzeBuffer()
{
  uint8_t i = 0;
  uint8_t ui8_resBufSize = 0;
  while(uint8_t j = ui8a_receiveBuffer[i++])
  {
    if(j != ' ')
    {
      j = Adjust(j);
      uint8_t k = ui8a_receiveBuffer[i++];
      if(k != ' ')
        ui8a_resultBuffer[ui8_resBufSize++] = j * 16 + Adjust(k);
      else
        ui8a_resultBuffer[ui8_resBufSize++] = j;
    }
  }

  // add checksum:
  uint8_t ui8_checkSum(0xFF);
  for(i = 0; i < ui8_resBufSize; i++)
    ui8_checkSum ^= ui8a_resultBuffer[i]; 
  bitWrite(ui8_checkSum, 7, 0);     // set MSB zero
  ui8a_resultBuffer[ui8_resBufSize++] = ui8_checkSum;

  return ui8_resBufSize;
}

#endif

//=== functions ==================================================
boolean GetPortValues(uint16_t* ui16_PortAddr, uint8_t* ui8_PortType)
{
  return decodeTelegramValues(ui16_PortAddr, ui8_PortType, ui8_Value1, ui8_Value2, ui8_Value3);
}

uint8_t GetWaitForTelegram() { return ui8_WaitForTelegram; }
void SetWaitForTelegram(uint8_t iType)
{
  ui8_WaitForTelegram = iType;
  ul_WaitStartForTelegram = millis();
}

void CheckWaitForTelegram()
{
  if(ui8_WaitForTelegram == 0x40)
  {
    // we are monitoring B0/B1/B2-telegrams
    return;
  }
  if((ui8_WaitForTelegram != 0) && ((millis() - ul_WaitStartForTelegram) > 5000))
  {
    if(ui8_WaitForTelegram != 0x80)
    {
      // we didn't receive an telegram for us within time:
      SetStoerung(ui8_WaitForTelegram << 8);
    }
    else
    {
      // Timeout for 'ReadAllModulAddresses()'
      b_ReadAllModulAddressesFinished = true;
    }
    ui8_WaitForTelegram &= ~ui8_WaitForTelegram;
    digitalWrite(PIN_LN_STATUS_LED, LOW); // communication has ended
  }
}

void InitLocoNet()
{
  pinMode(PIN_LN_STATUS_LED, OUTPUT);
  
  // First initialize the LocoNet interface, specifying the TX Pin
  LocoNet.init(PIN_TX);

#if defined TELEGRAM_FROM_SERIAL
  ClearReceiveBuffer();
#endif
}

uint8_t GetD1() { return ((LnPacket->data[5] & 0x01) << 7) + (LnPacket->data[6] & 0x7F); }
uint8_t GetD2() { return ((LnPacket->data[5] & 0x02) << 6) + (LnPacket->data[7] & 0x7F); }
uint8_t GetD3() { return ((LnPacket->data[5] & 0x04) << 5) + (LnPacket->data[8] & 0x7F); }
uint8_t GetD4() { return ((LnPacket->data[5] & 0x08) << 4) + (LnPacket->data[9] & 0x7F); }
uint8_t GetD5() { return ((LnPacket->data[10] & 0x01) << 7) + (LnPacket->data[11] & 0x7F); }
uint8_t GetD6() { return ((LnPacket->data[10] & 0x02) << 6) + (LnPacket->data[12] & 0x7F); }
uint8_t GetD7() { return ((LnPacket->data[10] & 0x04) << 5) + (LnPacket->data[13] & 0x7F); }
uint8_t GetD8() { return ((LnPacket->data[10] & 0x08) << 4) + (LnPacket->data[14] & 0x7F); }

void HandleLocoNetMessages()
{
#if defined TELEGRAM_FROM_SERIAL
  LnPacket = NULL;
  if (Serial.available())
  {
    byte byte_Read = Serial.read();
    if(byte_Read == '\n')
    {
      if(AnalyzeBuffer() > 0)
        LnPacket = (lnMsg*) (&ui8a_resultBuffer);
      ClearReceiveBuffer();
    }
    else
    {
      if(ui8_PointToBuffer < MAX_LEN_LNBUF)
        ui8a_receiveBuffer[ui8_PointToBuffer++] = byte_Read;
    }
  }
#else
  CheckWaitForTelegram();

  // Check for any received LocoNet packets
  LnPacket = LocoNet.receive();
#endif
  if(LnPacket)
  {
    if(ui8_WaitForTelegram)
    {
      LocoNet.processSwitchSensorMessage(LnPacket);

      // First print out the packet in HEX
      uint8_t ui8_msgLen = getLnMsgSize(LnPacket);
      uint8_t ui8_msgOPCODE = LnPacket->data[0];
      //====================================================
      if(ui8_msgLen == 16)
      {
        if (ui8_msgOPCODE == OPC_PEER_XFER) // E5
        {
          HandleE5MessageFormat1();
          HandleE5MessageFormat2();
        }
      } // if(ui8_msgLen == 16)
      //====================================================
    } // if(ui8_WaitForTelegram)
  } // if(LnPacket)
}

void HandleE5MessageFormat1()
{
  if(LnPacket->data[4] == SV2_Format_1)  // telegram for us with Message-Format '1'
  {
    if(LnPacket->data[3] == GetCV(ID_OWN))  // telegram for us
    {
      // check more telegram-data:
      if(   (LnPacket->data[2] == ui8_ModulAddr)
         && (GetD5() == ui8_ModulSubAddr)) // answer is from partner
      {
  #if defined DEBUG || defined TELEGRAM_FROM_SERIAL
        Printout('R');
  #endif
        if(   (LnPacket->data[6] == 0x01)             // answer for last write command
           && (LnPacket->data[7] == ui8_ModulSV))
        {
          if(ui8_WaitForTelegram == 0x01)
          {
            ui8_WaitForTelegram = 0x00;
            if(GetD8() == ui8_Value1)
            {
              // send second portvalue:
              SetWaitForTelegram(0x02);
              sendOneSV(ui8_ModulAddr, ui8_ModulSubAddr, ++ui8_ModulSV, ui8_Value2, true);
            }
            else
              SetFinishWithError(1);
          }
          else if(ui8_WaitForTelegram == 0x02)
          {
            ui8_WaitForTelegram = 0x00;
            if(GetD8() == ui8_Value2)
            {
              // send third portvalue:
              SetWaitForTelegram(0x03);
              sendOneSV(ui8_ModulAddr, ui8_ModulSubAddr, ++ui8_ModulSV, ui8_Value3, true);
            }
            else
              SetFinishWithError(2);
          }
          else if(ui8_WaitForTelegram == 0x03)
          {
            ui8_WaitForTelegram = 0x00;
            SetFinishWithError( GetD8() == ui8_Value3 ? 0 : 4);
          }
        } // if((LnPacket->data[6] == 0x01) && (LnPacket->data[7] == ui8_ModulSV))
        
        if(   (LnPacket->data[6] == 0x02)             // answer for last read command
           && (LnPacket->data[7] == ui8_ModulSV))
        {
          if(ui8_WaitForTelegram == 0x08)
          {
            ui8_WaitForTelegram = 0x00;
            ui8_Value1 = GetD6();  // SV x
            ui8_Value2 = GetD7();  // SV x+1
            ui8_Value3 = GetD8();  // SV x+2
            SetFinishWithError(0);
          }
        } // if((LnPacket->data[6] == 0x02) && (LnPacket->data[7] == ui8_ModulSV))
      } // if(   (LnPacket->data[2] == ui8_ModulAddr) && (ui8_currentSubAddress == ui8_ModulSubAddr))
      if(   (LnPacket->data[6] == 0x02)             // answer read command 'all adresses'
         && (LnPacket->data[7] == 0x00))
      {
        if(ui8_WaitForTelegram == 0x80)
        {
          // reveiving data from all Modules!
          if(ui8_MaxModulDataRead < MAX_MODULDATA)
          {
            // boolean b_Servo((GetD6() & 0x04) != 0 ? true : false);  // SV 0, not tested yet :-(
            st_ModulData[ui8_MaxModulDataRead].modulAddress.ui8_Address = GetD7();  // SV 1
            st_ModulData[ui8_MaxModulDataRead].modulAddress.ui8_SubAddress = GetD8(); // SV 2
            st_ModulData[ui8_MaxModulDataRead].ui8_Version = GetD3(); // SV 100?
            ++ui8_MaxModulDataRead;
          }
          else
          {
            // buffer full:
            b_ReadAllModulAddressesFinished = true;
            ui8_WaitForTelegram = 0x00;
            SetFinishWithError(0);
          }
        } // if(ui8_WaitForTelegram == 0x80)
      } // if((LnPacket->data[6] == 0x02) && (LnPacket->data[7] == 0x00))
    } // if(LnPacket->data[3] == GetCV(ID_OWN))
  } // if(LnPacket->data[4] == 0x01)
}

void workOnNotify(uint16_t Address, uint8_t Output, uint8_t Direction)
{
  if (ui8_WaitForTelegram == 0x40)
  {
#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
    Printout('R');
    PrintAdr(Address);
#endif
    if (Output)
    {
      b_NewTelegramAvailable = true;
      st_ModulData[ui8_MaxModulDataRead].ui16_ioAddress = Address;
      st_ModulData[ui8_MaxModulDataRead].ui8_State = Direction ? 0x10 : 0x00;  // "Grün" bzw. "Rot"
      ++ui8_MaxModulDataRead;
      if (ui8_MaxModulDataRead >= MAX_MODULDATA)
        ui8_MaxModulDataRead = 0; // Ringpuffer
    }
  }
}

void SetFinishWithError(uint8_t iError)
{
  ResetStoerung();
  if(!iError)
    ResetStoerung();
  else
    SetStoerung(iError);  
  digitalWrite(PIN_LN_STATUS_LED, LOW); // communication has ended
}

void InitModul()
{
  digitalWrite(PIN_LN_STATUS_LED, HIGH); // indicate that we are communicating
  ResetStoerung();
  SetWaitForTelegram(0x01);

  ui8_ModulSV = 0;                // Port 0, 1, 2
  ui8_Value1 = 0;                 // SV0
  ui8_Value2 = ui8_ModulAddr;     // SV1
  ui8_Value3 = ui8_ModulSubAddr;  // SV2

#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
  Serial.println(F("InitModul"));
#endif

  // Broadcast-Telegramm!
  sendOneSV(0, 0, 0, 0, true);
}

void ReadAllModulAddresses()
{
  digitalWrite(PIN_LN_STATUS_LED, HIGH); // indicate that we are communicating

  // prepare container for receiving data
  b_ReadAllModulAddressesFinished = false;
  ui8_MaxModulDataRead = 0;

  ModulData md;
  md.modulAddress.ui8_Address = md.modulAddress.ui8_SubAddress = md.ui8_Version = 0;
  for (uint8_t i = 0; i < MAX_MODULDATA; i++)
    st_ModulData[i] = md;
  // end prepare container for receiving data
  
  ResetStoerung();
  SetWaitForTelegram(0x80);

	// read(02) from PC(80) using Format1(01):
	// E5 10 80 00 01 00 02 00 00 00 00 00 00 00 00 <CHK>
  sendOneSV(0, 0, 0, 0, false);

#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
  Serial.println(F("ReadAllModulAddresses"));
#endif
}

void MonitorLocoIOStateTelegrams()
{
  // prepare container for receiving data
  b_ReadAllModulAddressesFinished = false;
  ui8_MaxModulDataRead = 0;

  for (uint8_t i = 0; i < MAX_MODULDATA; i++)
  {
    st_ModulData[i].ui16_ioAddress = 0;
    st_ModulData[i].ui8_State = 0;
  }
  // end prepare container for receiving data
  
  ResetStoerung();
  SetWaitForTelegram(0x40);

#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
  Serial.println(F("ReadAllLocoIOStateTelegrams"));
#endif
}

boolean HasNewTelegram() { return b_NewTelegramAvailable; }

boolean GetTelegram(uint16_t ui16_EditValue, uint16_t *ui16_LocoIOAddress, uint8_t *ui8_State)
{
  b_NewTelegramAvailable = false;
  *ui16_LocoIOAddress = 0;
  *ui8_State = 0;
  if((ui16_EditValue < MAX_MODULDATA) && (st_ModulData[ui16_EditValue].ui16_ioAddress > 0))
  {
    *ui16_LocoIOAddress = st_ModulData[ui16_EditValue].ui16_ioAddress;
    *ui8_State = st_ModulData[ui16_EditValue].ui8_State;
    return true;
  }
  return false;
}

boolean GetModulAddress(uint16_t ui16_EditValue, uint8_t *ui8_ModulAddress, uint8_t *ui8_ModulSubAddress, uint8_t *ui8_ModulVersion)
{
  *ui8_ModulAddress = *ui8_ModulSubAddress = *ui8_ModulVersion = 0;
  if((ui16_EditValue < MAX_MODULDATA) && (ui16_EditValue < ui8_MaxModulDataRead))
  {
    *ui8_ModulAddress = st_ModulData[ui16_EditValue].modulAddress.ui8_Address;
    *ui8_ModulSubAddress = st_ModulData[ui16_EditValue].modulAddress.ui8_SubAddress;
    *ui8_ModulVersion = st_ModulData[ui16_EditValue].ui8_Version;
    return true;
  }
  return false;
}

boolean ReadAllModulAddressesFinished()
{
  return b_ReadAllModulAddressesFinished;  
}

void ReadOnePortOfModule(uint8_t ui8_Addr, uint8_t ui8_SubAddr, uint8_t ui8_Port)
{
  digitalWrite(PIN_LN_STATUS_LED, HIGH); // indicate that we are communicating
  ResetStoerung();
  SetWaitForTelegram(0x08);
   
  ui8_ModulAddr = ui8_Addr;
  ui8_ModulSubAddr = ui8_SubAddr;
  ui8_ModulSV = ui8_Port * 3;

#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
  Serial.print(F("ReadOnePortOfModule: Addr:"));
  Serial.print(ui8_Addr);
  Serial.print(F(" Sub-Addr:"));
  Serial.print(ui8_SubAddr);
  Serial.print(F(" Port:"));
  Serial.print(ui8_Port);
  Serial.print(F(" (SV:"));
  Serial.print(ui8_ModulSV);
  Serial.println(')');
#endif

  sendOneSV(ui8_Addr, ui8_SubAddr, ui8_ModulSV, 0, false);
}

void WriteOnePortToModule(uint8_t ui8_Addr, uint8_t ui8_SubAddr, uint8_t ui8_Port, uint16_t ui16_PortAddr, uint8_t ui8_PortType)
{
  digitalWrite(PIN_LN_STATUS_LED, HIGH); // indicate that we are communicating
  ResetStoerung();
  SetWaitForTelegram(0x01);
   
  calculateTelegramValues(ui16_PortAddr, ui8_PortType, &ui8_Value1, &ui8_Value2, &ui8_Value3);

  ui8_ModulAddr = ui8_Addr;
  ui8_ModulSubAddr = ui8_SubAddr;
  ui8_ModulSV = ui8_Port * 3;

#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
  Serial.print(F("WriteOnePortToModule: Addr:"));
  Serial.print(ui8_Addr);
  Serial.print(F(" Sub-Addr:"));
  Serial.print(ui8_SubAddr);
  Serial.print(F(" Port:"));
  Serial.print(ui8_Port);
  Serial.print(F(" Port-Addr:"));
  Serial.print(ui16_PortAddr);
  Serial.print(F(" PortType:"));
  Serial.print(ui8_PortType);
  Serial.print('(');
  Serial.print(getPortTypeAsText(ui8_PortType));
  Serial.print(')');
  Serial.print(F(" SV"));
  Serial.print(ui8_ModulSV);
  Serial.print(':');
  Serial.print(ui8_Value1);
  Serial.print(F(" SV"));
  Serial.print(ui8_ModulSV + 1);
  Serial.print(':');
  Serial.print(ui8_Value2);
  Serial.print(F(" SV"));
  Serial.print(ui8_ModulSV + 2);
  Serial.print(':');
  Serial.println(ui8_Value3);
#endif

  sendOneSV(ui8_Addr, ui8_SubAddr, ui8_ModulSV, ui8_Value1, true);  // two more telegrams are send by 'HandleLocoNetMessages()'
}

void sendOneSV(uint8_t ui8_Addr, uint8_t ui8_SubAddr, uint8_t ui8_SV, uint8_t ui8_SVvalue, boolean b_Write)
{
  uint8_t SRC(GetCV(ID_OWN));
  uint8_t PXCT1((ui8_SVvalue & 0x80) >> 4);
  uint8_t D1(b_Write ? 0x01 : 0x02);
  
  // calculate checksum:
  uint8_t ui8_ChkSum = OPC_PEER_XFER ^ 0x10 ^ SRC ^ ui8_Addr ^ SV2_Format_1 ^ PXCT1 ^ D1 ^ ui8_SV ^ (ui8_SVvalue & 0x7F) ^ ui8_SubAddr ^ 0xFF;  //XOR
  bitWrite(ui8_ChkSum, 7, 0);     // set MSB zero

  addByteLnBuf( &LnTxBuffer, OPC_PEER_XFER);      //OPCODE: 0xE5
  addByteLnBuf( &LnTxBuffer, 0x10);               //LEN: length
  addByteLnBuf( &LnTxBuffer, SRC);                //SRC: Source-ID
  addByteLnBuf( &LnTxBuffer, ui8_Addr);           //DST: send to modul-addr
  addByteLnBuf( &LnTxBuffer, SV2_Format_1);       //Message-Format: 1 (fixed value)
  addByteLnBuf( &LnTxBuffer, PXCT1);              //PXCT1: calculated above
  addByteLnBuf( &LnTxBuffer, D1);                 //D1: command: write(0x01) or read(0x02)
  addByteLnBuf( &LnTxBuffer, ui8_SV);             //D2: SV number to write
  addByteLnBuf( &LnTxBuffer, 0x00);               //D3: fixed value
  addByteLnBuf( &LnTxBuffer, ui8_SVvalue & 0x7F); //D4: SV value to write
  addByteLnBuf( &LnTxBuffer, 0x00);               //PXCT2: fixed value
  addByteLnBuf( &LnTxBuffer, ui8_SubAddr);        //D5: send to modul-subaddr
  addByteLnBuf( &LnTxBuffer, 0x00);               //D6: fixed value
  addByteLnBuf( &LnTxBuffer, 0x00);               //D7: fixed value
  addByteLnBuf( &LnTxBuffer, 0x00);               //D8: fixed value
  addByteLnBuf( &LnTxBuffer, ui8_ChkSum );        //Checksum
  addByteLnBuf( &LnTxBuffer, 0xFF );              //Limiter

  // Check to see if we have received a complete packet yet
  LnPacket = recvLnMsg( &LnTxBuffer );    //Prepare to send
  if(LnPacket)
  { // check correctness
    LocoNet.send( LnPacket );  // Send the received packet from the PC to the LocoNet
#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
    Printout('T');
#endif
  }
}

#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
void Printout(char ch)
{
  if(LnPacket)
  {
    // print out the packet in HEX
    Serial.print(ch);
    Serial.print(F("X: "));
    uint8_t ui8_msgLen = getLnMsgSize(LnPacket); 
    for (uint8_t i = 0; i < ui8_msgLen; i++)
    {
      uint8_t ui8_val = LnPacket->data[i];
      // Print a leading 0 if less than 16 to make 2 HEX digits
      if(ui8_val < 16)
        Serial.print('0');
      Serial.print(ui8_val, HEX);
      Serial.print(' ');
    }
    Serial.println();
  } // if(LnPacket)
}
void PrintAdr(uint16_t ui16_AdrFromTelegram)
{
  Serial.print(F("Adr received from LN"));
  Serial.println(ui16_AdrFromTelegram);
}
#endif
