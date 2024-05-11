//=== Keypad === usable for all ====================================
#include <Wire.h>
#include <i2ckeypad.h>

//=== declaration of var's =======================================
#define PCF8574T_ADDR_KEY 0x21
#define PCF8574A_ADDR_KEY (PCF8574T_ADDR_KEY + 0x18)

#if not defined BUTTON_SELECT
  #define BUTTON_UP 0x08
  #define BUTTON_DOWN 0x04
  #define BUTTON_LEFT 0x10
  #define BUTTON_RIGHT 0x02
  #define BUTTON_SELECT 0x01
#endif

#define BUTTON_KEYPAD 0x80
#define BUTTON_STAR   0x40

uint8_t ui8_KPDi2cAddress(PCF8574A_ADDR_KEY);
i2ckeypad kpd = i2ckeypad(ui8_KPDi2cAddress);  // default: 4 Rows, 4 Cols

uint8_t ui8_KPDPresent = 0;  // ui8_KPDPresent: 1 if keypad is found

boolean bShowFrediSV(false);
uint16_t ui16_ThrottleId(0);

//=== functions ==================================================
uint8_t isKeypadPresent () { return ui8_KPDPresent; }
uint8_t getKPDAdddress () { return ui8_KPDPresent ? ui8_KPDi2cAddress : 0; }
boolean getKey(uint8_t* ui8_char)
{
  if(ui8_KPDPresent)
  {
    char key(kpd.get_key());  // returns non-zero after key released
    if(key != '\0')
    {
      *ui8_char = key;
      return true;
    }
  }
  return false;
}

void CheckAndInitKeypad()
{
  Wire.begin();

  //---keypad--------------
  ui8_KPDi2cAddress = PCF8574A_ADDR_KEY;
  Wire.beginTransmission(ui8_KPDi2cAddress);
  boolean b_keypadDetected(Wire.endTransmission() == 0);

  bool bAddrT(false);
  if(!b_keypadDetected)
  {
    ui8_KPDi2cAddress = PCF8574T_ADDR_KEY;
    Wire.beginTransmission(ui8_KPDi2cAddress);
    b_keypadDetected = (Wire.endTransmission() == 0);
    bAddrT = true;
  }
  
  if(b_keypadDetected && !ui8_KPDPresent)
  {
    // keypad (newly) found:
    // set up the keypad with columns and rows: 
    kpd.setAddr(ui8_KPDi2cAddress);
    kpd.init();

#if defined DEBUG
    Serial.print(F("Keypad found at address "));
    Serial.println(ui8_KPDi2cAddress); 
#endif
  } // if(b_keypadDetected && !ui8_KPDPresent)
  ui8_KPDPresent = (b_keypadDetected ? 1 : 0);
}

void HandleKeypad()
{
  CheckAndInitKeypad();
}

void getEditValueFromKeypad(boolean b_Edit, uint16_t ui16_Max, uint16_t* ui16_Value, uint8_t* ui8_buttons)
{
  if(ui8_KPDPresent)
  {
    char key(kpd.get_key());  // returns non-zero after key released
    if(key != '\0')
    {
      if(key == '#')
      {
        *ui8_buttons |= BUTTON_SELECT;
        return;
      }
      if(key == '*')
      {
        *ui8_buttons |= BUTTON_STAR;
        return;
      }
      if(!b_Edit)
        return;
      key -= '0';
      if(key == 18) // 'B'
        *ui16_Value = (*ui16_Value > 10 ? *ui16_Value/10 : 0);
      else if(key == 19) // 'C'
        *ui16_Value = 0;
      else if((key >= 0) && (key <= 9)) // '0'...'9'
      {
        uint16_t ui16Temp((*ui16_Value * 10) + key);
        *ui16_Value = ui16Temp > ui16_Max ? *ui16_Value : ui16Temp;
      }
      *ui8_buttons |= BUTTON_KEYPAD;
      return;
    }
  }
  return;
}

