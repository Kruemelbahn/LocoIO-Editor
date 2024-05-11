//=== CV-Routines === usable for all ==============================
#include <EEPROM.h>

//=== functions ==================================================
uint8_t GetCVCount() { return MAX_CV; }

boolean IsCVUI8(uint8_t ui8_Index) { return ((ui8_Index < MAX_CV) && (cvDefinition[ui8_Index].TYPE == CV_TYPE::UI8) ? true : false); }

boolean IsCVUI16(uint8_t ui8_Index) { return ((ui8_Index < MAX_CV) && (cvDefinition[ui8_Index].TYPE == CV_TYPE::UI16) ? true : false); }

boolean IsCVBinary(uint8_t ui8_Index) { return ((ui8_Index < MAX_CV) && (cvDefinition[ui8_Index].TYPE == CV_TYPE::BINARY) ? true : false); }

uint16_t GetCVMinValue(uint8_t ui8_Index) { return (ui8_Index < MAX_CV ? cvDefinition[ui8_Index].MIN : 0); }

uint16_t GetCVMaxValue(uint8_t ui8_Index) { return (ui8_Index < MAX_CV ? cvDefinition[ui8_Index].MAX : UINT16_MAX); }

uint16_t GetCV(uint8_t ui8_Index) { return (ui8_Index < MAX_CV ? ui16a_CV[ui8_Index] : UINT16_MAX); }

boolean CanEditCV(uint8_t ui8_Index) { return (ui8_Index < MAX_CV ? !cvDefinition[ui8_Index].RO : false); }

void SetCVsToDefault()
{
#if defined DEBUG
    Serial.println(F("initalizing CVs"));
#endif

    EEPROM.write(0, char('M'));
    EEPROM.write(1, char('Z'));

    for (uint8_t ui8_Index = 0; ui8_Index < MAX_CV; ui8_Index++)
      WriteCVtoEEPROM(cvDefinition[ui8_Index].ID, cvDefinition[ui8_Index].DEFLT);

#if defined DEBUG
  #if defined DEBUG_CV
    PrintCurrentCVs();
  #endif  
#endif
}

void WriteCVtoEEPROM(uint8_t ui8_Index, uint16_t ui16_Value)
{
  if ((ui8_Index < MAX_CV) && (ui16_Value <= cvDefinition[ui8_Index].MAX))
  {
    ui16a_CV[ui8_Index] = ui16_Value;

    uint8_t ui8_EEPROM_Position((ui8_Index + 1) * 2); // index starts with Zero! keep space for 'MZ'
    EEPROM.write(ui8_EEPROM_Position, (uint8_t)(ui16_Value >> 8));        // MSB
    EEPROM.write(ui8_EEPROM_Position + 1, (uint8_t)(ui16_Value & 0xFF));  // LSB
  }
}

boolean CheckAndWriteCVtoEEPROM(uint8_t ui8_Index, uint16_t ui16_Value)
{
  if (ui8_Index == VERSION_NUMBER)
  {
    if (ui16_Value == 0)
      SetCVsToDefault();
    else
      WriteCVtoEEPROM(VERSION_NUMBER, SW_VERSION);  // reset to default
    return true;
  }

  boolean b_WriteCV(!cvDefinition[ui8_Index].RO);
  if ((ui16_Value < cvDefinition[ui8_Index].MIN) || (ui16_Value > cvDefinition[ui8_Index].MAX))
    b_WriteCV = false;
  if (b_WriteCV)
  {
    WriteCVtoEEPROM(ui8_Index, ui16_Value);
    return true;
  }
  return false;
}

void ReadCVsFromEEPROM()
{
  // start reading from the first byte (address 2) of the EEPROM
  // remember: 0 resp. 1 should contain 'MZ'
  for (uint8_t ui8_Index = 0; ui8_Index < MAX_CV; ui8_Index++)
  {
    uint8_t ui8_EEPROM_Position((ui8_Index + 1) * 2);

    uint8_t ui8_MSB(EEPROM.read(ui8_EEPROM_Position));
    uint8_t ui8_LSB(EEPROM.read(ui8_EEPROM_Position + 1));
    ui16a_CV[ui8_Index] = (uint16_t)((ui8_MSB << 8) + ui8_LSB);
  }

  // check if current Version is in EEPROM:
  if(ui16a_CV[VERSION_NUMBER] < SW_VERSION)
    // write current Version into EEPROM:
    WriteCVtoEEPROM(VERSION_NUMBER, SW_VERSION);
  
#if defined DEBUG
  #if defined DEBUG_CV
    PrintCurrentCVs();
  #endif  
#endif

  if(!AlreadyCVInitialized())  // initialising has to be done
    SetCVsToDefault();
}

#if defined DEBUG
  #if defined DEBUG_CV
    void PrintCurrentCVs()
    {
      Serial.println(F("current CVs"));
      for(uint8_t i = 0; i < MAX_CV; i++)
      {
        Serial.print(F("CV "));
        Serial.print(i+1);
        Serial.print(F(": "));
        Serial.println(ui16a_CV[i]);
      }
    }
  #endif  
#endif
