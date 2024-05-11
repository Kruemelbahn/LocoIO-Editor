//=== LocoIOConfig for LocoIO ===

#define MAXPORTTYPE 35       // Anzahl der Porttypen

uint8_t getMaxPortType() { return MAXPORTTYPE; }

const __FlashStringHelper* getPortTypeAsText(uint8_t iPortType)
{
  const __FlashStringHelper* portType[MAXPORTTYPE] = 
    {  F("BlckDet.low    "), // 0
       F("BlckDet.low del"),
       F("BlckDet.hig    "),
       F("BlckDet.hig del"),
       F("Toggle Swit.   "),
       F("Toggle Swit.ind"), // 5
       F("PushBtn.low    "),
       F("PushBtn.low ind"),
       F("PushBtn.hig    "),
       F("PushBtn.hig ind"),
       F("SwitchPnt.Feedb"), // 10
       F("Cont1.Pnt.Feedb"),
       F("Cont2.Pnt.Feedb"),
       F("Output occup.  "),
       F("Output occup.bl"),
       F("Fix.out1 on    "), // 15
       F("Fix.out1 on bl "),
       F("Fix.out1 on   4"),
       F("Fix.out1 on bl4"),
       F("Fix.out1 of    "),
       F("Fix.out1 of bl "), // 20
       F("Fix.out1 of   4"),
       F("Fix.out1 of bl4"),
       F("Fix.out2 on    "),
       F("Fix.out2 on bl "),
       F("Fix.out2 on   4"), // 25
       F("Fix.out2 on bl4"),
       F("Fix.out2 of    "),
       F("Fix.out2 of bl "),
       F("Fix.out2 of   4"),
       F("Fix.out2 of bl4"), // 30
       F("Puls1 SoftReset"),
       F("Puls1 HardReset"),
       F("Puls2 SoftReset"),
       F("Puls2 HardReset"), // 34
    };

  if(iPortType < MAXPORTTYPE)
    return (portType[iPortType]);
  else
    return (F("???"));
}

static const uint8_t valArray[MAXPORTTYPE][3] =
  { { 31,  16, 1, },  // 0
    { 27,  16, 1, },
    { 95,   0, 1, },
    { 91,   0, 1, },
    { 15,  16, 0, },
    {  7,  16, 0, }, // 5
    { 47,  16, 1, },
    { 39,  16, 1, },
    {111,   0, 1, },
    {103,   0, 1, },
    { 23, 112, 0, },  // 10
    { 55, 112, 0, },
    { 55,  96, 0, },
    {192,   0, 1, },
    {208,   0, 1, },
    {129,  16, 0, },  // 15
    {145,  16, 0, },
    {161,  16, 0, },
    {177,  16, 0, },
    {128,  16, 0, },
    {144,  16, 0, },  // 20
    {160,  16, 0, },
    {176,  16, 0, },
    {129,  48, 0, },
    {145,  48, 0, },
    {161,  48, 0, },  // 25  
    {177,  48, 0, },
    {128,  48, 0, },
    {144,  48, 0, },
    {160,  48, 0, },
    {176,  48, 0, },  // 30
    {136,  32, 0, },
    {140,  32, 0, },
    {136,   0, 0, },
    {140,   0, 0, }  // 34
  };

void calculateTelegramValues(uint16_t ui16_PortAddr, uint8_t ui8_PortType, uint8_t* iValue1, uint8_t* iValue2, uint8_t* iValue3)
{
  *iValue1 = *iValue2 = *iValue3 = 0;
  if((ui8_PortType < MAXPORTTYPE) && (ui16_PortAddr > 0))
  {
    uint16_t ui16_Adr(ui16_PortAddr);
    boolean b_AdrEven(false);
    switch(valArray[ui8_PortType][2])
    {
      case 0 : --ui16_Adr; break;
      case 1 : b_AdrEven = ui16_PortAddr & 0x01 ? false : true;
               ui16_Adr = (ui16_Adr - 1 - (b_AdrEven ? 1 : 0)) >> 1; break;
      default : return;
    }
    *iValue1 = valArray[ui8_PortType][0];
    *iValue2 = ui16_Adr & 0x007F;
    *iValue3 = ((ui16_Adr & 0xFF80) >> 7) + valArray[ui8_PortType][1] + (b_AdrEven ? 0x20 : 0);
  }
}

boolean decodeTelegramValues(uint16_t* ui16_PortAddr, uint8_t* ui8_PortType, uint8_t iValue1, uint8_t iValue2, uint8_t iValue3)
{
  *ui16_PortAddr = *ui8_PortType = 0;
  uint16_t ui16_Adr = (iValue2 & 0x7F) + ((iValue3 & 0x0F) << 7);
  for(uint8_t i = 0; i < MAXPORTTYPE; i++)
  {
    if(iValue1 == valArray[i][0])
    {
      if((iValue3 & ((valArray[i][2] == 1) ? 0xD0 : 0xF0)) == valArray[i][1])
      {
        switch(valArray[i][2])
        {
          case 0 : *ui16_PortAddr = ui16_Adr + 1; break;
          case 1 : *ui16_PortAddr = (ui16_Adr * 2) + 1 + (iValue3 & 0x20 ? 1 : 0); break;
          default : return false;
        }
        *ui8_PortType = i;
        return true;
      }
    }
  }
  return false;
}
