//=== LocoNet support for E5-Telegram === usable for all =============
#if defined ENABLE_LN_E5
#include <LocoNet.h>

//=== functions ==================================================
boolean TelegramE5ForUs()
{
#if defined FREDI_NG
  if ((LnPacket->data[6] == GetCV(SV_THROTTLE_ID_L)) &&	// 7-bit
    (LnPacket->data[7] == GetCV(SV_THROTTLE_ID)))  // 7-bit : telegram for us
#else
	if ((LnPacket->data[6] == GetCV(ID_DEVICE)) &&	// 7-bit
		(LnPacket->data[7] == GetCV(SOFTWARE_ID)))  // 7-bit : telegram for us
#endif
    return true;
	return false;
}

/*
* CVs used by me are always starting with number 1, e.g. CV1, CV2 a.s.o.
*    but index for GetCV/WriteCVtoEEPROM starts with 0
* 
* changed (2022-08) behaviour: my CVs are always (internaly) 2-Bytes
* therfore one CV needs up to two transferbytes: D1 (& D2) / LSB (& MSB), depending whether CV-Type is type 'ui16' or not!
* 
* 16-bit-CVs seemed to be currently (2022-08-27) supported by JMRI; I'm not sure how to define this in the xml-files...
* see:  ...\JMRI\java\src\jmri\jmrix\loconet\LnOpsModeProgrammer.java
*       line 510 =  void loadSV2MessageFormat(LocoNetMessage m, int mAddress, int cvAddr, int data) {
*       ...
*       line 536 =  m.setElement(11, data&0xFF);
*                   m.setElement(12, (data>>8)&0xFF);
*                   m.setElement(13, (data>>16)&0xFF);
*                   m.setElement(14, (data>>24)&0xFF);
*
*/
void HandleE5MessageFormat2()
{
  if (LnPacket->data[4] == SV2_Format_2)  // telegram with Message-Format '2'
  {
    if (TelegramE5ForUs())
    {
      // SV_ADRL und SV_ADRH
      uint8_t ui8_LSBAdr(((LnPacket->data[5] & 0x04) << 5) + (LnPacket->data[8] & 0x7F));
      uint8_t ui8_MSBAdr(((LnPacket->data[5] & 0x08) << 4) + (LnPacket->data[9] & 0x7F));
      uint16_t ui16_Address((ui8_MSBAdr << 8) + ui8_LSBAdr);

      uint16_t ui16_CVAddress(ui16_Address - 1);
      boolean bIsCVUI16(IsCVUI16((uint8_t)ui16_CVAddress));

      if (LnPacket->data[3] == 0x01)  // SV write: write 1 CV (one resp. two byte) of data from D1 (& D2)
      {
        if (ui16_CVAddress < GetCVCount())
        {
          uint8_t ui8_valueLSB(((LnPacket->data[10] & 0x01) << 7) + (LnPacket->data[11] & 0x7F));    // D1
          uint8_t ui8_valueMSB(0);
          // CAUTION:
          // depending on sender of telegram, D2 may be zero even in case of 16-bit-CV-Value.
          // in this case the upper byte of CV maight be set to zero, even if this was not desired!
          if(bIsCVUI16)
            ui8_valueMSB = ((LnPacket->data[10] & 0x02) << 6) + (LnPacket->data[12] & 0x7F);         // D2
          uint16_t ui16_CVValue((ui8_valueMSB << 8) + ui8_valueLSB);
          WriteCVtoEEPROM((uint8_t)ui16_CVAddress, ui16_CVValue);

          // REPLY SV write: transfers a write response in D1 (& D2)
          // CAUTION:
          // D2 may contain a value in case of 16-bit-CV-Value.
          // depending on receiver of telegram, D2 is probably ignored!
          LnPacket->data[3] |= 0x40;
          ui16_CVValue = GetCV((uint8_t)ui16_CVAddress);
          ui8_valueLSB = (uint8_t)(ui16_CVValue & 0xFF);  // D1
          ui8_valueMSB = (uint8_t)(ui16_CVValue << 8);    // D2
          sendE5Telegram((uint8_t)(ui16_Address & 0x00FF), (uint8_t)(ui16_Address >> 8), ui8_valueLSB, ui8_valueMSB, 0, 0);
        }
      } // if (LnPacket->data[3] == 0x01)  // SV write: write 1 CV (one resp. two byte) of data from D1 (& D2)

      if (LnPacket->data[3] == 0x02)  // SV read: initiate read 1 CV (one resp. two byte) of data into D1 (& D2)
      {
        if (ui16_CVAddress < GetCVCount())
        {
          // REPLY SV read : transfers a read response in D1 (& D2)
          // CAUTION:
          // D2 may contain a value in case of 16-bit-CV-Value.
          // depending on receiver of telegram, D2 is probably ignored!
          LnPacket->data[3] |= 0x40;
          uint16_t ui16_CVValue(GetCV((uint8_t)ui16_CVAddress));
          uint16_t ui8_valueLSB((uint8_t)(ui16_CVValue & 0xFF));  // D1
          uint16_t ui8_valueMSB((uint8_t)(ui16_CVValue << 8));    // D2
          sendE5Telegram((uint8_t)(ui16_Address & 0x00FF), (uint8_t)(ui16_Address >> 8), ui8_valueLSB, ui8_valueMSB, 0, 0);
        }
      } // if (LnPacket->data[3] == 0x02)  // SV read: initiate read 1 CV (one resp. two byte) of data into D1 (& D2)

      if (LnPacket->data[3] == 0x03)  // SV write: write 1 CV masked (value in D1, bit-mask in D2)
      {
        if (ui16_CVAddress < GetCVCount())
        {
          uint16_t ui16_value(((LnPacket->data[10] & 0x01) << 7) + (LnPacket->data[11] & 0x7F));    // D1
          uint16_t ui16_BitMask(((LnPacket->data[10] & 0x02) << 6) + (LnPacket->data[12] & 0x7F));  // D2

          // CAUTION:
          // works only with the lower byte of any CV!
          uint16_t ui16_CVValue(GetCV((uint8_t)ui16_CVAddress));
          if (ui16_value & ui16_BitMask)
            ui16_CVValue |= ui16_BitMask;
          else
            ui16_CVValue &= ~ui16_BitMask;

          WriteCVtoEEPROM((uint8_t)ui16_CVAddress, ui16_CVValue);

          // REPLY SV write: transfers a write response in D1 (& D2)
          // CAUTION:
          // D2 may contain a value in case of 16-bit-CV-Value.
          // depending on receiver of telegram, D2 is probably ignored!
          LnPacket->data[3] |= 0x40;
          ui16_CVValue = GetCV((uint8_t)ui16_CVAddress);
          uint16_t ui8_valueLSB((uint8_t)(ui16_CVValue & 0xFF));  // D1
          uint16_t ui8_valueMSB((uint8_t)(ui16_CVValue << 8));    // D2
          sendE5Telegram((uint8_t)(ui16_Address & 0x00FF), (uint8_t)(ui16_Address >> 8), ui8_valueLSB, ui8_valueMSB, 0, 0);
        }
      } // if (LnPacket->data[3] == 0x03)  // SV write: write 1 CV masked (value in D1, bit-mask in D2)

      if (LnPacket->data[3] == 0x05)  // SV write: write 4 CV (one byte each) of data from D1...D4
      {
        if (ui16_CVAddress < GetCVCount())
        {
          // CAUTION:
          // this function only works correctly with 1-Byte-CV.
          // in case of 16-bit-CV, the upper byte of CV is set to zero, even if this was not desired!
          uint8_t ui8_value_D1(((LnPacket->data[10] & 0x01) << 7) + (LnPacket->data[11] & 0x7F));    // D1
          uint8_t ui8_value_D2(((LnPacket->data[10] & 0x02) << 6) + (LnPacket->data[12] & 0x7F));    // D2
          uint8_t ui8_value_D3(((LnPacket->data[10] & 0x04) << 5) + (LnPacket->data[13] & 0x7F));    // D3
          uint8_t ui8_value_D4(((LnPacket->data[10] & 0x08) << 4) + (LnPacket->data[14] & 0x7F));    // D4
          WriteCVtoEEPROM((uint8_t)ui16_CVAddress, (uint16_t)ui8_value_D1);
          WriteCVtoEEPROM((uint8_t)(ui16_CVAddress + 1), (uint16_t)ui8_value_D2);
          WriteCVtoEEPROM((uint8_t)(ui16_CVAddress + 2), (uint16_t)ui8_value_D3);
          WriteCVtoEEPROM((uint8_t)(ui16_CVAddress + 3), (uint16_t)ui8_value_D4);

          // REPLY SV write: transfers a write response in D1...D4
          // CAUTION:
          // this function only works correctly with 1-Byte-CV.
          // in case of 16-bit-CV,the upper byte of CV is ignored and would not be sended!
          LnPacket->data[3] |= 0x40;
          ui8_value_D1 = (uint8_t)(GetCV((uint8_t)ui16_CVAddress) & 0x00FF);
          // following values may be 0xFF if ui16_CVAddress >= MAX_CV
          ui8_value_D2 = (uint8_t)(GetCV((uint8_t)ui16_CVAddress + 1) & 0x00FF);
          ui8_value_D3 = (uint8_t)(GetCV((uint8_t)ui16_CVAddress + 2) & 0x00FF);
          ui8_value_D4 = (uint8_t)(GetCV((uint8_t)ui16_CVAddress + 3) & 0x00FF);
          sendE5Telegram((uint8_t)(ui16_Address & 0x00FF), (uint8_t)(ui16_Address >> 8), ui8_value_D1, ui8_value_D2, ui8_value_D3, ui8_value_D4);
        }
      } // if (LnPacket->data[3] == 0x05)  // SV write: write 4 CV (one byte each) of data from D1...D4

      if (LnPacket->data[3] == 0x06)  // SV read: initiate read 4 CV (one byte each) of data into D1...D4
      {
        if (ui16_CVAddress < GetCVCount())
        {
          // REPLY SV read : transfers a read response in D1...D4
          // CAUTION:
          // this function only works correctly with 1-Byte-CV.
          // in case of 16-bit-CV,the upper byte of CV is ignored and would not be sended!
          LnPacket->data[3] |= 0x40;
          uint8_t ui8_value_D1((uint8_t)(GetCV((uint8_t)ui16_CVAddress) & 0x00FF));
          // following values may be 0xFF if ui16_CVAddress >= MAX_CV
          uint8_t ui8_value_D2((uint8_t)(GetCV((uint8_t)ui16_CVAddress + 1) & 0x00FF));
          uint8_t ui8_value_D3((uint8_t)(GetCV((uint8_t)ui16_CVAddress + 2) & 0x00FF));
          uint8_t ui8_value_D4((uint8_t)(GetCV((uint8_t)ui16_CVAddress + 3) & 0x00FF));
          sendE5Telegram((uint8_t)(ui16_Address & 0x00FF), (uint8_t)(ui16_Address >> 8), ui8_value_D1, ui8_value_D2, ui8_value_D3, ui8_value_D4);
        }
      } // if (LnPacket->data[3] == 0x06)  // SV read: initiate read 4 CV (one byte each) of data into D1...D4

      if (LnPacket->data[3] == 0x08)  // Identify: causes an individual device to identify itself
      {
        // REPLY Identify: transfers an Identify response
        LnPacket->data[3] |= 0x40;
#if defined FREDI_NG
        sendE5Telegram(DEVELOPER_ID, MANUFACTURER_ID, GetCV(SV_PRODUCT_SWID), 0, GetCV(SV_THROTTLE_ID), GetCV(SV_THROTTLE_ID_L));
#else
        sendE5Telegram(DEVELOPER_ID, MANUFACTURER_ID, GetCV(PRODUCT_ID), 0, GetCV(ID_DEVICE), GetCV(SOFTWARE_ID));
#endif
      } // if (LnPacket->data[3] == 0x08)  // Identify: causes an individual device to identify itself

      if (LnPacket->data[3] == 0x0F)  // Reconfigure: initiates a device reconfiguration or reset
      {
        SetCVsToDefault();
        // REPLY Reconfigure: Acknowledgement immediately
        LnPacket->data[3] |= 0x40;
        sendE5Telegram((uint8_t)(ui16_Address & 0x00FF), (uint8_t)(ui16_Address >> 8), 0, 0, 0, 0);
      } // if (LnPacket->data[3] == 0x0F)  // Reconfigure: initiates a device reconfiguration or reset
    } // if (TelegramE5ForUs())

     // broadcast:
    if ((LnPacket->data[6] == 0) && (LnPacket->data[7] == 0)) 
    {
      if (LnPacket->data[3] == 0x07)  // Discover: causes all devices to identify themselves
      {
        LnPacket->data[3] |= 0x40;
#if defined FREDI_NG
        sendE5Telegram(DEVELOPER_ID, MANUFACTURER_ID, GetCV(SV_PRODUCT_SWID), 0, GetCV(SV_THROTTLE_ID), GetCV(SV_THROTTLE_ID_L));
#else
        sendE5Telegram(DEVELOPER_ID, MANUFACTURER_ID, GetCV(PRODUCT_ID), 0, GetCV(ID_DEVICE), GetCV(SOFTWARE_ID));
#endif
      } // if (LnPacket->data[3] == 0x07)
    } // if ((LnPacket->data[6] == 0) && (LnPacket->data[7] == 0))  // broadcast
  } // if (LnPacket->data[4] == SV2_Format_2)  // telegram with Message-Format '2'
}

boolean sendE5Telegram(uint8_t ui8_sv_adrl, uint8_t ui8_sv_adrh, uint8_t ui8_D1, uint8_t ui8_D2, uint8_t ui8_D3, uint8_t ui8_D4)
{
  uint8_t svx1(LnPacket->data[5] & 0x03); // keep bits from dst_l & dst_h
  return sendE5Telegram(LnPacket->data[2], LnPacket->data[3], svx1, LnPacket->data[6], LnPacket->data[7], ui8_sv_adrl, ui8_sv_adrh, ui8_D1, ui8_D2, ui8_D3, ui8_D4);
}

boolean sendE5Telegram(uint8_t src, uint8_t cmd, uint8_t svx1, uint8_t dst_l, uint8_t dst_h, uint8_t ui8_sv_adrl, uint8_t ui8_sv_adrh, uint8_t ui8_D1, uint8_t ui8_D2, uint8_t ui8_D3, uint8_t ui8_D4)
{
  svx1 |= 0b00010000;
  uint8_t adrl(ui8_sv_adrl & 0x7F);
  if (ui8_sv_adrl > 0x7F)
    svx1 |= 0b00000100;
  uint8_t adrh(ui8_sv_adrh & 0x7F);
  if (ui8_sv_adrh > 0x7F)
    svx1 |= 0b00001000;

  uint8_t SVX2(0b00010000);

  uint8_t D1(ui8_D1 & 0x7F);
  if (ui8_D1 > 0x7F)
    SVX2 |= 0b00000001;
  uint8_t D2(ui8_D2 & 0x7F);
  if (ui8_D2 > 0x7F)
    SVX2 |= 0b00000010;
  uint8_t D3(ui8_D3 & 0x7F);
  if (ui8_D3 > 0x7F)
    SVX2 |= 0b00000100;
  uint8_t D4(ui8_D4 & 0x7F);
  if (ui8_D4 > 0x7F)
    SVX2 |= 0b00001000;

  // calculate checksum:
  uint8_t ui8_ChkSum(OPC_PEER_XFER ^ 16 ^ src ^ cmd ^ SV2_Format_2 ^ svx1 ^ dst_l ^ dst_h ^ adrl ^ adrh ^ SVX2 ^ D1 ^ D2 ^ D3 ^ D4 ^ 0xFF);  //XOR
  bitWrite(ui8_ChkSum, 7, 0);     // set MSB zero

  addByteLnBuf(&LnTxBuffer, OPC_PEER_XFER);				// opcode E5
  addByteLnBuf(&LnTxBuffer, 16);									// length
  addByteLnBuf(&LnTxBuffer, src);		              // src
  addByteLnBuf(&LnTxBuffer, cmd);		              // cmd
  addByteLnBuf(&LnTxBuffer, SV2_Format_2);				// sv-type
  addByteLnBuf(&LnTxBuffer, svx1);								// svx1
  addByteLnBuf(&LnTxBuffer, dst_l);		            // dst_l    
  addByteLnBuf(&LnTxBuffer, dst_h);		            // dst_h
  addByteLnBuf(&LnTxBuffer, adrl);					      // sv_adrl    
  addByteLnBuf(&LnTxBuffer, adrh);					      // sv_adrh
  addByteLnBuf(&LnTxBuffer, SVX2);								// svx2
  addByteLnBuf(&LnTxBuffer, D1);									// D1
  addByteLnBuf(&LnTxBuffer, D2);									// D2
  addByteLnBuf(&LnTxBuffer, D3);									// D3
  addByteLnBuf(&LnTxBuffer, D4);									// D4
  addByteLnBuf(&LnTxBuffer, ui8_ChkSum);					// Checksum
  addByteLnBuf(&LnTxBuffer, 0xFF);								// Limiter

  { // send packet
    // Check to see if we have received a complete packet yet
    LnPacket = recvLnMsg(&LnTxBuffer);    //Prepare to send
    if (LnPacket)
    { // check correctness
      LocoNet.send(LnPacket);  // Send the received packet from the PC to the LocoNet
#if defined DEBUG || defined TELEGRAM_FROM_SERIAL 
      Printout('T');
#endif
    }
    return true;
  }
}
#endif // #if defined ENABLE_LN_E5
