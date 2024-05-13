//=== LCDPanel for LocoIO ===
#if defined LCD

#include <Wire.h>
#include <LCDPanel.h>
#include <LocoNetKS.h>

//=== declaration of var's =======================================
//--- for use of LCD
#define CTRL_BUTTON 5
#define BUTTON_KEYPAD 0x80

LCDPanel displayPanel = LCDPanel();

uint8_t ui8_DisplayPanelPresent = 0;  // ui8_DisplayPanelPresent: 1 if I2C-LCD-Panel is found
//--- end LCD

#define B0_OFF          ((GetCV(ADD_FUNCTIONS_1) & 0x01) == 0x01)
#define ENABLE_INIT_PIC ((GetCV(ADD_FUNCTIONS_1) & 0x80) == 0x80)

/* mode:
  0   after init "LocoIO-SV-Edit" is displayed
  1   "Status?" is displayed
  2   "Inbetriebnahme?" is displayed
  5   "Stoerung" is displayed
  6   "Beobachten?" is displayed
  7   "Steuern?" is displayed
  
 20   (edit) mode for CV1
 21   (edit) mode for CV2
 22   (edit) mode for CV3
 23   (edit) mode for CV4
 24   (edit) mode for CV5
 25   (edit) mode for CV6
 26   (edit) mode for CV7
 27   (edit) mode for CV8
 28   (edit) mode for CV9

 70   "SV-Editor?"
 71   "Adresse"
 72   "Adress-Abfrage?"
 73   Adressliste
 75   "Sub-Adressse"
 76   Anzeige der Daten eines Portes
 77   "Portadresse"
 78   "Art des Ports"
 79   "Daten an Port speichern?"
 80   "wird gesendet..."
 81   Stoerungsquittierung
 82   warten auf Rückmeldung
 83   warten auf Ende Moduladressen lesen

 90   "LocoIO-Init?"
 91   "nur ein LocoIO angeschlossen?
 92   "wird initialisiert..."
 
100   IO-Telegramm-Anzeige

110   Adresseingabe
111   Telegrammauswahl
112   Statusauswahl
113   B0(aus)

200   confirm display CV's
210   confirm display I2C-Scan
211   I2C-Scan

215   Tastatur-Test

220   display IP-Address
221   display MAC-Address

*/
uint8_t ui8_DisplayPanelMode = 0;  // ui8_DisplayPanelMode for paneloperation

uint8_t ui8_ButtonMirror = 0;
uint16_t ui16_StoerungMirror = 65535;
boolean b_CtrlMirror(false);

unsigned long ul_Wait = 0;

static const uint8_t MAX_MODE = 19 + GetCVCount();
#define MAXMODUL 127
#define MAXSUBMODUL 126
#define MAXPORT 16
#define MAXPORTADR 2048

uint16_t ui16_EditValue = 0;
boolean b_Edit = false;

uint8_t ui8_CursorX = 0;

// used for sv-edit:
uint8_t   ui8_modulAddress = 0;
uint8_t   ui8_modulSubAddress = 0;
uint8_t   ui8_modulPortNumber = 1;
uint16_t  ui16_modulPortAddress = 0;
uint8_t   ui8_modulPortType = 0;

// used for telegram:
uint16_t  ui16_SwitchAddress = 0;
uint8_t   ui8_TelegramKind = 0;
boolean   b_SwitchState = false;

// used for LN-Monitor
boolean   isLNMonitorActive() { return false; }

//=== functions ==================================================
void CheckAndInitDisplayPanel()
{
  //---LCD-----------------
  boolean b_DisplayPanelDetected(displayPanel.detect_i2c(MCP23017_ADDRESS) == 0);
  if(b_DisplayPanelDetected && !ui8_DisplayPanelPresent)
  {
    pinMode(CTRL_BUTTON, INPUT_PULLUP);
    
    // LCD (newly) found:
    // set up the LCD's number of columns and rows: 
    displayPanel.begin(16, 2);

#if defined DEBUG
    Serial.println(F("LCD-Panel found..."));
    Serial.print(F("..."));
    Serial.println(GetSwTitle());
#endif

    if(AlreadyCVInitialized())
      OutTextTitle();

    ui8_DisplayPanelMode = 0;
    ui8_ButtonMirror = 0;
    ui16_EditValue = 0;
    ui8_CursorX = 0;
    ui8_modulPortType = 0;
    ui8_ButtonMirror = 0;
    ui8_modulAddress = 0;
    ui8_modulSubAddress = 0;
    ui16_modulPortAddress = 0;
    ui8_modulPortNumber = 1;
    ui16_StoerungMirror = 65535;
    b_Edit = false;
  } // if(b_DisplayPanelDetected && !ui8_DisplayPanelPresent)
  ui8_DisplayPanelPresent = (b_DisplayPanelDetected ? 1 : 0);
}

void DisplayFirstLine(const __FlashStringHelper *fText)
{
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  displayPanel.print(fText);
}

void DisplaySecondLine(const __FlashStringHelper *fText)
{
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
  displayPanel.print(fText);
}

void SetDisplayPanelModeStatus()
{ // mode: 1
  DisplayFirstLine(F("Status?"));
  ui8_DisplayPanelMode = 1;
}

void SetDisplayPanelModeIBN()
{ // mode: 2
  DisplayFirstLine(F("Inbetriebnahme?"));
  ui8_DisplayPanelMode = 2;
}

void SetDisplayPanelModeWatchIOTelegram()
{ // mode: 6
  DisplayFirstLine(F("Beobachten?"));
  ui8_DisplayPanelMode = 6;
}

void SetDisplayPanelModeControl()
{ // mode: 7
  DisplayFirstLine(F("Steuern?"));
  ui8_DisplayPanelMode = 7;
  b_Edit = false;
}

void SetDisplayPanelModeSVEdit()
{ // mode: 70
  DisplayFirstLine(F("SV-Editor?"));
  ui8_DisplayPanelMode = 70;
  b_Edit = false;
}

void SetDisplayPanelModeSVModulAddress()
{ // mode: 71
  DisplayFirstLine(F("Modul-Adresse"));
  ui8_DisplayPanelMode = 71;
  b_Edit = true;
  ui16_EditValue = ui8_modulAddress;
  displayValue3(ui8_modulAddress);
}

void SetDisplayPanelModeSVRequestModules()
{ // mode: 72
  DisplayFirstLine(F("Modul-Adressen?"));
  ui8_DisplayPanelMode = 72;
  b_Edit = false;
}

void SetDisplayPanelModeSVModulSubAddress()
{ // mode: 75
  DisplayFirstLine(F("Modul-SubAdresse"));
  ui8_DisplayPanelMode = 75;
  ui16_EditValue = ui8_modulSubAddress;
  displayValue3(ui8_modulSubAddress);
}

void LimitPortNumber(uint16_t& ui16Value)
{
#if defined DEBUG
	Serial.print(F("current DisplayPanelMode: "));
	Serial.println(ui8_DisplayPanelMode);
	Serial.print(F("LimitPortNumber: "));
	Serial.println(ui16Value);
#endif
	if(!ui16Value)
		ui16Value = 1;
	else
		if(ui16Value > MAXPORT)
			ui16Value = MAXPORT;
}

void SetDisplayPanelModeSVModulPortnumber()
{ // mode: 76
  DisplayFirstLine(F("Port:"));
  ui8_DisplayPanelMode = 76;
  boolean b_State(GetPortValues(&ui16_modulPortAddress, &ui8_modulPortType));
  ui16_EditValue = ui8_modulPortNumber;
	LimitPortNumber(ui16_EditValue);
  decout(displayPanel, (uint8_t)(ui16_EditValue & 0xFF), 2);
  displayPanel.setCursor(8, 0);  // set the cursor to column x, line y
  displayPanel.print(F("Adr:"));
  if(b_State)
    decout(displayPanel, ui16_modulPortAddress, 4);
  else
    displayPanel.print(F("???"));
  b_Edit=false;
  displayPortTypeAsText(b_State ? ui8_modulPortType : 255);
  b_Edit=true;
}

void SetDisplayPanelModeSVModulPortAddress()
{ // mode: 77
  DisplayFirstLine(F("Neue Adresse"));
  ui8_DisplayPanelMode = 77;
  ui16_EditValue = ui16_modulPortAddress;
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
  if(b_Edit)
    displayPanel.print('>');
  decout(displayPanel, ui16_EditValue, 4);
}

void SetDisplayPanelModeSVModulPortType()
{ // mode: 78
  DisplayFirstLine(F("Neuer Typ"));
  ui8_DisplayPanelMode = 78;
  b_Edit = true;
  ui16_EditValue = ui8_modulPortType;
  displayPortTypeAsText(ui16_EditValue);
}

void SetDisplayPanelModeRequestSendToPort()
{ // mode: 79
  b_Edit = false;
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  decout(displayPanel, (uint8_t)ui8_modulAddress, 3);
  displayPanel.print('.');
  decout(displayPanel, (uint8_t)ui8_modulSubAddress, 3);
  displayPanel.print(':');
  decout(displayPanel, (uint8_t)ui8_modulPortNumber, 2);
  displayPanel.print('=');
  decout(displayPanel, ui16_modulPortAddress, 4);
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
  decout(displayPanel, (uint8_t)ui8_modulPortType, 2);
  displayPanel.print(F(" -> speichern?"));
  ui8_DisplayPanelMode = 79;
}

void SetDisplayPanelStoerungBeiUebertragung()
{ // mode: 81
  DisplayFirstLine(F("Fehler bei"));
  ui8_DisplayPanelMode = 81;
  displayPanel.setCursor(0,1);
  displayPanel.print(F("Kommunikation!"));
}

void SetDisplayPanelUebertragung()
{ // mode: 82
  DisplayFirstLine(F("wird gesendet..."));
  ui8_DisplayPanelMode = 82;
}

void SetDisplayPanelRunning()
{ // mode: 83
  DisplayFirstLine(F("Abfrage laeuft"));
  ui8_DisplayPanelMode = 83;
  ul_Wait = millis();
  ui16_EditValue = 0;
}

void SetDisplayPanelModeInitModule()
{ // mode: 90
  DisplayFirstLine(F("LocoIO-Init?"));
  ui8_DisplayPanelMode = 90;
  b_Edit = false;
}

void SetDisplayPanelModePICAlone()
{ // mode: 91
  DisplayFirstLine(F("nur ein LocoIO"));
  DisplaySecondLine(F("angeschlossen?"));
  ui8_DisplayPanelMode = 91;
}

void SetDisplayPanelInitialisierung()
{ // mode: 92
  DisplayFirstLine(F("LocoIO wird"));
  DisplaySecondLine(F("initialisiert..."));
  ui8_DisplayPanelMode = 92;
}

void SetDisplayPanelModePortAddress()
{ // mode: 110
  DisplayFirstLine(F("Port-Adresse"));
  ui8_DisplayPanelMode = 110;
  b_Edit = true;
  ui16_EditValue = ui16_SwitchAddress;
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
  displayPanel.print('>');
  decout(displayPanel, ui16_EditValue, 4);
}

void SetDisplayPanelModeTelegramType()
{ // mode: 111
  DisplayFirstLine(F("Telegramm-Art"));
  ui8_DisplayPanelMode = 111;
  ui16_EditValue = ui8_TelegramKind;
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
  if(b_Edit)
    displayPanel.print('>');
  displayPanel.print('B');
  displayPanel.print(ui16_EditValue);
}

void SetDisplayPanelModeTelegramState()
{ // mode: 112
  DisplayFirstLine(F("Ausgangsstatus"));
  ui8_DisplayPanelMode = 112;
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
  if(b_Edit)
    displayPanel.print('>');
  displayPanel.print(b_SwitchState ? F("ein"): F("aus"));
}

void SetDisplayPanelModeCV()
{ // mode: 200
  DisplayFirstLine(F("CV?"));
  ui8_DisplayPanelMode = 200;
}

void SetDisplayPanelModeScan()
{ // mode: 210
  DisplayFirstLine(F("I2C-Scan?"));
  ui8_DisplayPanelMode = 210;
}

void SetDisplayPanelModeKeypadTest()
{ // mode: 215
  DisplayFirstLine(F("Tastatur-Test"));
  ui8_DisplayPanelMode = 215;
}

#if defined ETHERNET_BOARD
void SetDisplayPanelModeIpAdr()
{ // mode: 220
  DisplayFirstLine(F("IP-Adresse"));
  DisplaySecondLine(Ethernet.localIP());
  ui8_DisplayPanelMode = 220;
}

void SetDisplayPanelModeMacAdr()
{ // mode: 221
  DisplayFirstLine(F("MAC-Adresse"));
  ui8_DisplayPanelMode = 221;
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
  uint8_t *mac(GetMACAddress());
  for(uint8_t i = 0; i < 6; i++)
  {
    hexout(displayPanel, (uint8_t)(mac[i]), 2);
    if((i == 1) || (i == 3))
      displayPanel.print('.');
  }
}
#endif

void InitScan()
{
  displayPanel.setCursor(8, 0);  // set the cursor to column x, line y
  displayPanel.print(' ');       // clear '?'
  ui16_EditValue = -1;
  NextScan();
}

void NextScan()
{
  ++ui16_EditValue;
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y

  if (ui16_EditValue > 127)
  {
    OutTextFertig();
    return;
  }

  uint8_t error(0);
  do
  {
    Wire.beginTransmission((uint8_t)(ui16_EditValue));
    error = Wire.endTransmission();

    if ((error == 0) || (error == 4))
    {
      displayPanel.print(F("0x"));
      hexout(displayPanel, (uint8_t)(ui16_EditValue), 2);
      displayPanel.print((error == 4) ? '?' : ' ');
      break;
    }

    ++ui16_EditValue;
    if (ui16_EditValue > 127)
    { // done:
      OutTextFertig();
      break;
    }
  } while (true);
}

void OutModulData(uint8_t ui8_ModulAddress, uint8_t ui8_ModulSubAddress, uint8_t ui8_ModulVersion)
{
  decout(displayPanel, (uint8_t)ui8_ModulAddress, 3);
  displayPanel.print('/');
  decout(displayPanel, (uint8_t)ui8_ModulSubAddress, 3);
  displayPanel.print(F(" V"));
  decout(displayPanel, (uint8_t)ui8_ModulVersion, 3);
}

void NextModul()
{
  displayPanel.clear();
  displayPanel.setCursor(0, 0);
  ++ui16_EditValue;
  uint8_t ui8_ModulAddress, ui8_ModulSubAddress, ui8_ModulVersion; 
  if(GetModulAddress(ui16_EditValue, &ui8_ModulAddress, &ui8_ModulSubAddress, &ui8_ModulVersion))
  {
    OutModulData(ui8_ModulAddress, ui8_ModulSubAddress, ui8_ModulVersion);
    displayPanel.setCursor(0, 1);
    if(GetModulAddress(ui16_EditValue + 1, &ui8_ModulAddress, &ui8_ModulSubAddress, &ui8_ModulVersion))
      OutModulData(ui8_ModulAddress, ui8_ModulSubAddress, ui8_ModulVersion);
    else
      OutTextFertig();
  }
  else
    OutTextFertig();
}

void PreviousModul()
{
  displayPanel.clear();
  displayPanel.setCursor(0, 0);
  if(ui16_EditValue > 0)
    --ui16_EditValue;
  uint8_t ui8_ModulAddress, ui8_ModulSubAddress, ui8_ModulVersion; 
  if(GetModulAddress(ui16_EditValue, &ui8_ModulAddress, &ui8_ModulSubAddress, &ui8_ModulVersion))
  {
    OutModulData(ui8_ModulAddress, ui8_ModulSubAddress, ui8_ModulVersion);
    displayPanel.setCursor(0, 1);
    if(GetModulAddress(ui16_EditValue + 1, &ui8_ModulAddress, &ui8_ModulSubAddress, &ui8_ModulVersion))
      OutModulData(ui8_ModulAddress, ui8_ModulSubAddress, ui8_ModulVersion);
    else
      OutTextFertig();
  }
  else
    OutTextFertig();
}

void OutTextFertig() { displayPanel.print(F("fertig")); }
void OutTextStoerung() { displayPanel.print(F("Stoerung")); }

void OutTextTitle()
{
  DisplayFirstLine(GetSwTitle()); 
  DisplaySecondLine(F("Version "));
  displayPanel.print(GetCV(VERSION_NUMBER));
#if defined DEBUG
  displayPanel.print(F(" Deb"));
#endif
#if defined TELEGRAM_FROM_SERIAL
  displayPanel.print(F("Sim"));
#endif
}

void displayValue3(uint8_t iValue)
{
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
  if(b_Edit)
    displayPanel.print('>');
  decout(displayPanel, (uint8_t)iValue, 3);
}

void displayValue4(uint16_t iValue)
{
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
  if(b_Edit)
    displayPanel.print('>');
  decout(displayPanel, iValue, 4);
}

void DisplayCV(uint16_t ui16_Value)
{
  displayPanel.noCursor();
  displayPanel.clear();
  displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
  displayPanel.print(F("CV"));
  displayPanel.setCursor(3, 0);
  uint8_t ui8CvNr(ui8_DisplayPanelMode - 19);
  if (ui8CvNr < 10)
    displayPanel.print(' ');
  displayPanel.print(ui8CvNr);
  --ui8CvNr;

  // show shortname:
  displayPanel.setCursor(6, 0);
  displayPanel.print(GetCVName(ui8CvNr));

  displayPanel.setCursor(0, 1);
  if (b_Edit)
    displayPanel.print('>');
  if (IsCVBinary(ui8CvNr))
    binout(displayPanel, ui16_Value);
  else
    decout(displayPanel, ui16_Value, GetCountOfDigits(ui8CvNr));
  if (!b_Edit && !CanEditCV(ui8CvNr))
  {
    displayPanel.setCursor(14, 1);
    displayPanel.print(F("ro"));
  }
  if (b_Edit && CanEditCV(ui8CvNr))
  {
    displayPanel.setCursor(ui8_CursorX, 1);
    displayPanel.cursor();
  }
}

uint8_t GetCountOfDigits(uint8_t ui8CvNr)
{
  uint16_t ui16MaxCvValue(GetCVMaxValue(ui8CvNr));
  if (ui16MaxCvValue > 9999)
    return 5;
  if (ui16MaxCvValue > 999)
    return 4;
  if (ui16MaxCvValue > 99)
    return 3;
  if (ui16MaxCvValue > 9)
    return 2;
  return 1;
}

uint16_t GetFactor(uint8_t ui8CvNr)
{
  uint16_t ui16_faktor(0);
  uint8_t ui8_Position(GetCountOfDigits(ui8CvNr));
  switch (ui8_Position - ui8_CursorX)
  {
  case 0: ui16_faktor = 1; break;
  case 1: ui16_faktor = 10; break;
  case 2: ui16_faktor = 100; break;
  case 3: ui16_faktor = 1000; break;
  case 4: ui16_faktor = 10000; break;
  }
  return ui16_faktor;
}

void displayPortTypeAsText(uint8_t iPortType)
{
  displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
  if(b_Edit)
    displayPanel.print('>');
  if(iPortType < getMaxPortType())
    displayPanel.print(getPortTypeAsText(iPortType));
  else
    displayPanel.print(F("???"));
}

void OutTelegramData(uint16_t ui16_EditValue, uint16_t ui16_LocoIOAddress, uint8_t ui8_State)
{
  decout(displayPanel, (uint8_t)(ui16_EditValue & 0xFF), 3);
  displayPanel.print(F(" B"));
  displayPanel.print(char((ui8_State & 0x07) + '0'));
  displayPanel.print(' ');
  decout(displayPanel, ui16_LocoIOAddress, 4);
  if((ui8_State & 0x10) == 0x10)
    displayPanel.print(F(" on "));
  else
    displayPanel.print(F(" off"));
}

void DisplayTelegrams()
{
  displayPanel.clear();
  displayPanel.setCursor(0, 0);
  uint16_t ui16_LocoIOAddress;
  uint8_t ui8_State; 
  if(GetTelegram(ui16_EditValue, &ui16_LocoIOAddress, &ui8_State))
  {
    OutTelegramData(ui16_EditValue, ui16_LocoIOAddress, ui8_State);
    displayPanel.setCursor(0, 1);
    if(GetTelegram(ui16_EditValue + 1, &ui16_LocoIOAddress, &ui8_State))
      OutTelegramData((ui16_EditValue + 1), ui16_LocoIOAddress, ui8_State);
    else
      decout(displayPanel, (uint8_t)((ui16_EditValue + 1) & 0xFF), 3);
  }
  else
    decout(displayPanel, (uint8_t)(ui16_EditValue & 0xFF), 3);
}

void HandleDisplayPanel()
{
  CheckAndInitDisplayPanel();

  if(ui8_DisplayPanelPresent != 1)
  {
    SetWaitForTelegram(0);
    return;
  }

  uint8_t ui8_bs(1);
  if(displayPanel.readButtonA5() == BUTTON_A5)
    ui8_bs = 0;
  displayPanel.setBacklight(ui8_bs);

  uint8_t ui8_buttons = displayPanel.readButtons();  // reads only 5 buttons (A0...A4)

#if defined ENABLE_KEYPAD
  // --- keypad-support ---
  uint16_t iMax(UINT8_MAX);
  if(ui8_DisplayPanelMode == 71)
    iMax = MAXMODUL;
  if(ui8_DisplayPanelMode == 75)
    iMax = MAXSUBMODUL;
  if(ui8_DisplayPanelMode == 76)
    iMax = MAXPORT;
  if(ui8_DisplayPanelMode == 77)
    iMax = MAXPORTADR;
  if(ui8_DisplayPanelMode == 110)
    iMax = MAXPORTADR;
  if(ui8_DisplayPanelMode == 111)
    iMax = 2;
  if(ui8_DisplayPanelMode == 112)
    iMax = 1;
  if(ui8_DisplayPanelMode != 215) // Tastatur-Test
    getEditValueFromKeypad(b_Edit && (ui8_DisplayPanelMode != 78), iMax, &ui16_EditValue, &ui8_buttons); // overides BUTTON_SELECT
  // --- end of keypad-support ---
#endif

  if(!ui8_buttons && (ui8_ButtonMirror != ui8_buttons))
  {
    ui8_ButtonMirror = 0;
    return;
  }

  if(ui8_buttons && (ui8_ButtonMirror != ui8_buttons))
  {
    ui8_ButtonMirror = ui8_buttons;
    if(ui8_DisplayPanelMode == 0)
    {
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to SV-Editor
        SetDisplayPanelModeSVEdit();  // mode = 70
      }
      return;
    } // if(ui8_DisplayPanelMode == 0)
    //------------------------------------
    if(ui8_DisplayPanelMode == 1)
    {
      // actual ui8_DisplayPanelMode = "Status?"
      if (ui8_buttons & BUTTON_UP)
      { // switch to Control
        SetDisplayPanelModeControl();  // mode = 7
      }
      else if (ui8_buttons & BUTTON_DOWN)
      { // switch to IBN
        SetDisplayPanelModeIBN(); // mode = 2
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // show disturbance
        displayPanel.clear();
        displayPanel.setCursor(0, 0);  // set the cursor to column x, line y
        OutTextStoerung();
        ui8_DisplayPanelMode = 5;
      }
      return;
    } // if(ui8_DisplayPanelMode == 1)
    //------------------------------------
    if(ui8_DisplayPanelMode == 2)
    {
      // actual ui8_DisplayPanelMode = "IBN?"
      if (ui8_buttons & BUTTON_UP)
      { // switch to Status
        SetDisplayPanelModeStatus();  // mode = 1
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to ask CV / I²C-Scan
        SetDisplayPanelModeCV();  // mode = 200
      }
      return;
    } // if(ui8_DisplayPanelMode == 2)
    //------------------------------------
    if(ui8_DisplayPanelMode == 5)
    {
      // actual ui8_DisplayPanelMode = "Stoerung"
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to IBN
        SetDisplayPanelModeStatus();  // mode = 1
      } // if (ui8_buttons & BUTTON_RIGHT)
      return;
    } // if(ui8_DisplayPanelMode == 5)
    //------------------------------------
    if(ui8_DisplayPanelMode == 6)
    {
      // actual ui8_DisplayPanelMode = "Beobachten?"
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to Control
        SetDisplayPanelModeControl();  // mode = 7
      }
      else if (ui8_buttons & BUTTON_UP)
      { // switch to request addresses
        SetDisplayPanelModeSVRequestModules();  // mode = 72
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to collect IO-Telegrams
        displayPanel.clear();
        ui8_DisplayPanelMode = 100;
        ui16_EditValue = 0;
        MonitorLocoIOStateTelegrams();
        DisplayTelegrams();
      }
      return;
    } // if(ui8_DisplayPanelMode == 6)
    //------------------------------------
    if(ui8_DisplayPanelMode == 7)
    {
      // actual ui8_DisplayPanelMode = "Steuern?"
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to IBN
        SetDisplayPanelModeStatus();  // mode = 1
      }
      else if (ui8_buttons & BUTTON_UP)
      { // switch to Beobachten?
        SetDisplayPanelModeWatchIOTelegram();  // mode = 6
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to send IO-Telegrams
        SetDisplayPanelModePortAddress(); // mode = 110
      }
      return;
    } // if(ui8_DisplayPanelMode == 7)
    //------------------------------------
    if ((ui8_DisplayPanelMode >= 20) && (ui8_DisplayPanelMode <= MAX_MODE))
    {
      uint8_t ui8CurrentCvNr(ui8_DisplayPanelMode - 20);
      // actual ui8_DisplayPanelMode = view CV
      if (ui8_buttons & BUTTON_SELECT)
      { // save value for current CV
        if (b_Edit)
        {
          // save current value
          boolean b_OK(CheckAndWriteCVtoEEPROM(ui8CurrentCvNr, ui16_EditValue));
          DisplayCV(GetCV(ui8CurrentCvNr));
          displayPanel.setCursor(10, 1);  // set the cursor to column x, line y
          displayPanel.print(b_OK ? F("stored") : F("failed"));
          displayPanel.setCursor(ui8_CursorX, 1);
    }
        return;
  }
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to display current CV's
        if (b_Edit)
        {
          // leave edit-mode (without save)
          b_Edit = false;
          DisplayCV(GetCV(ui8CurrentCvNr));
          return;
        }
        SetDisplayPanelModeCV();
        return;
      }
      if (ui8_buttons & BUTTON_RIGHT)
      { // switch to edit current CV's
        if (!b_Edit && CanEditCV(ui8CurrentCvNr))
        {
          // enter edit-mode
          ui16_EditValue = GetCV(ui8CurrentCvNr);
          b_Edit = true;
          ui8_CursorX = 0;
          DisplayCV(ui16_EditValue);
        }
        if (b_Edit)
        { // move cursor only in one direction
          ++ui8_CursorX;

          // rollover:
          if (IsCVBinary(ui8CurrentCvNr))
          {
            if (ui8_CursorX == 9)
              ui8_CursorX = 1;
          }
          else
          {
            if (ui8_CursorX == (GetCountOfDigits(ui8CurrentCvNr) + 1))
              ui8_CursorX = 1;
          }

          displayPanel.setCursor(ui8_CursorX, 1);
          displayPanel.cursor();
        }
        return;
      }
      if (ui8_buttons & BUTTON_UP)
      {
        if (b_Edit)
        { // Wert vergrößern:
          uint16_t ui16_CurrentValue(ui16_EditValue);
          if (IsCVUI8(ui8CurrentCvNr) || IsCVUI16(ui8CurrentCvNr))
          {
            ui16_EditValue += GetFactor(ui8CurrentCvNr);
            if (ui16_EditValue < ui16_CurrentValue)
              ui16_EditValue = ui16_CurrentValue;  // Überlauf
          }

          if (IsCVBinary(ui8CurrentCvNr))
            ui16_EditValue |= (1 << (8 - ui8_CursorX));

          if (ui16_EditValue > GetCVMaxValue(ui8CurrentCvNr))
            ui16_EditValue = ui16_CurrentValue;  // Überlauf

          DisplayCV(ui16_EditValue);
          return;
        }
        // switch to next CV
        ++ui8_DisplayPanelMode;
        if (ui8_DisplayPanelMode >= MAX_MODE)
          ui8_DisplayPanelMode = MAX_MODE;
        DisplayCV(GetCV(ui8_DisplayPanelMode - 20)); // ui8_DisplayPanelMode has changed :-)
        return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      {
        if (b_Edit)
        { // Wert verkleinern:
          uint16_t ui16_CurrentValue(ui16_EditValue);
          if (IsCVUI8(ui8CurrentCvNr) || IsCVUI16(ui8CurrentCvNr))
          {
            ui16_EditValue -= GetFactor(ui8CurrentCvNr);
            if (ui16_EditValue > ui16_CurrentValue)
              ui16_EditValue = ui16_CurrentValue; // Unterlauf
          }

          if (IsCVBinary(ui8CurrentCvNr))
            ui16_EditValue &= ~(1 << (8 - ui8_CursorX));

          if (ui16_EditValue < GetCVMinValue(ui8CurrentCvNr))
            ui16_EditValue = ui16_CurrentValue; // Unterlauf

          if (ui16_EditValue == UINT16_MAX) // 0 -> 255/65535
            ui16_EditValue = ui16_CurrentValue; // Unterlauf

          DisplayCV(ui16_EditValue);
          return;
        }
        // switch to previous CV
        --ui8_DisplayPanelMode;
        if (ui8_DisplayPanelMode < 20)
          ui8_DisplayPanelMode = 20;
        DisplayCV(GetCV(ui8_DisplayPanelMode - 20)); // ui8_DisplayPanelMode has changed :-)
        return;
      }
    } // if((ui8_DisplayPanelMode >= 20) && (ui8_DisplayPanelMode <= MAX_MODE))
    //------------------------------------
    if(ui8_DisplayPanelMode == 70)
    {
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to request addresses
        SetDisplayPanelModeSVRequestModules();  // mode = 72
      }
      else if (ui8_buttons & BUTTON_UP)
      { // switch to Title
        OutTextTitle();
        ui8_DisplayPanelMode = 0;
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to ask for modul-address
        SetDisplayPanelModeSVModulAddress();  // mode = 71
      }
      return;
    } // if(ui8_DisplayPanelMode == 70)
    //------------------------------------
    if(ui8_DisplayPanelMode == 71)
    {
#if defined ENABLE_KEYPAD
      if (ui8_buttons & BUTTON_KEYPAD)
      {
        displayValue3((uint8_t)(ui16_EditValue & 0xFF));
      }
      else 
#endif
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to IBN
        ui16_EditValue = ui8_modulAddress;
        SetDisplayPanelModeSVEdit();  // mode = 70
      }
      else if (ui8_buttons & BUTTON_SELECT)
      { // switch to ask for modul-subaddress
        ui8_modulAddress = (uint8_t)(ui16_EditValue & 0xFF);
        if(   (ui8_modulAddress > 0 )
           && (ui8_modulAddress <= MAXMODUL) 
           && (ui8_modulAddress != ADR_MAIN_CONTROLLER) 
           && (ui8_modulAddress != GetCV(ID_OWN)))
          SetDisplayPanelModeSVModulSubAddress(); // mode = 75
      }
      else if(b_Edit)
      {
        if (ui8_buttons & BUTTON_UP)
        { 
          // Wert verkleinern:
          --ui16_EditValue;
          if(ui16_EditValue < 1)
            ui16_EditValue = 1;
          displayValue3((uint8_t)(ui16_EditValue & 0xFF));
        }
        else if (ui8_buttons & BUTTON_DOWN)
        { 
          // Wert vergrößern:
          ++ui16_EditValue;
          if(ui16_EditValue > MAXMODUL)
            ui16_EditValue = MAXMODUL;
          displayValue3((uint8_t)(ui16_EditValue & 0xFF));
        }
      }
      return;
    } // if(ui8_DisplayPanelMode == 71)
    //------------------------------------
    if(ui8_DisplayPanelMode == 72)
    {
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to Beobachten?
        SetWaitForTelegram(0);  // Telegramm-Mitschnitt beenden
        SetDisplayPanelModeWatchIOTelegram();  // mode = 6
      }
      else if (ui8_buttons & BUTTON_UP)
      { // switch to IBN
        ui16_EditValue = ui8_modulAddress;
        SetDisplayPanelModeSVEdit();  // mode = 70
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // send telegram for requesting modules
        ReadAllModulAddresses();
        SetDisplayPanelRunning();  // mode = 83
      }
      return;
    } // if(ui8_DisplayPanelMode == 72)
    //------------------------------------
    if(ui8_DisplayPanelMode == 73)
    {
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to request addresses
        SetDisplayPanelModeSVRequestModules();  // mode = 72
      }
      else if (ui8_buttons & BUTTON_UP)
      {
        PreviousModul();
      }
      else if (ui8_buttons & BUTTON_DOWN)
      {
        NextModul();
      }
      return;
    } // if(ui8_DisplayPanelMode == 73)
    //------------------------------------
    if(ui8_DisplayPanelMode == 75)
    {
      if (ui8_buttons & BUTTON_KEYPAD)
      {
        displayValue3((uint8_t)(ui16_EditValue & 0xFF));
      }
      else if (ui8_buttons & BUTTON_LEFT)
      { // // switch to ask for modul-address
        SetDisplayPanelModeSVModulAddress();  // mode = 71
      }
      else if (ui8_buttons & BUTTON_SELECT)
      { // switch to ask for modul-port
        ui8_modulSubAddress = (uint8_t)(ui16_EditValue & 0xFF);
        if(   (ui8_modulSubAddress > 0 ) 
           && (ui8_modulSubAddress <= MAXSUBMODUL))
           {
              ui8_modulPortNumber = 1;
              if(ENABLE_INIT_PIC && (digitalRead(CTRL_BUTTON) == LOW))
              {
                SetDisplayPanelModeInitModule();  // mode = 90
              }
              else
              {
                SetDisplayPanelUebertragung();  // mode = 82
                ReadOnePortOfModule(ui8_modulAddress, ui8_modulSubAddress, ui8_modulPortNumber);
              }
           }
      }
      else if (ENABLE_INIT_PIC && (ui8_buttons & 0x40))  // Button '*'
      {
        SetDisplayPanelModeInitModule();  // mode = 90
      }
      else if(b_Edit)
      {
        if (ui8_buttons & BUTTON_UP)
        { 
          // Wert verkleinern:
          --ui16_EditValue;
          if(ui16_EditValue < 1)
            ui16_EditValue = 1;
          displayValue3((uint8_t)(ui16_EditValue & 0xFF));
        }
        if (ui8_buttons & BUTTON_DOWN)
        { 
          // Wert vergrößern:
          ++ui16_EditValue;
          if(ui16_EditValue > MAXSUBMODUL)
            ui16_EditValue = MAXSUBMODUL;
          displayValue3((uint8_t)(ui16_EditValue & 0xFF));
        }
      }
      return;
    } // if(ui8_DisplayPanelMode == 75)
    //------------------------------------
    if(ui8_DisplayPanelMode == 76)
    {
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to ask for modul-subaddress
        ui16_EditValue = ui8_modulSubAddress;
        SetDisplayPanelModeSVModulSubAddress(); // mode = 75
      }
      else if (ui8_buttons & BUTTON_SELECT)
      { // switch to ask for new port-address
				LimitPortNumber(ui16_EditValue);
        ui8_modulPortNumber = (uint8_t)(ui16_EditValue & 0xFF);
        ui16_EditValue = ui16_modulPortAddress;
        SetDisplayPanelModeSVModulPortAddress();  // mode = 77
      }
      else if(b_Edit)
      {
        boolean b_Send(false);
        if (ui8_buttons & BUTTON_KEYPAD)
        {
          b_Send = true;
        }
        else if (ui8_buttons & BUTTON_UP)
        { 
          // Wert verkleinern:
          --ui16_EditValue;
          b_Send = true;
        }
        else if (ui8_buttons & BUTTON_DOWN)
        { 
          // Wert vergrößern:
          ++ui16_EditValue;
          b_Send = true;
        }
        if(b_Send)
        {
					LimitPortNumber(ui16_EditValue);
          ui8_modulPortNumber = (uint8_t)(ui16_EditValue & 0xFF);

          displayPanel.setCursor(5, 0);  // set the cursor to column x, line y
          decout(displayPanel, (uint8_t)ui8_modulPortNumber, 2);

          SetDisplayPanelUebertragung();
          ReadOnePortOfModule(ui8_modulAddress, ui8_modulSubAddress, ui8_modulPortNumber);
        }
      }
      return;
    } // if(ui8_DisplayPanelMode == 76)
    //------------------------------------
    if(ui8_DisplayPanelMode == 77)
    {
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to ask for modul-Port
        SetDisplayPanelModeSVModulPortnumber(); // mode = 76
      }
      else if (ui8_buttons & BUTTON_SELECT)
      { // switch to ask for new port-type
        ui16_modulPortAddress = ui16_EditValue;
        ui16_EditValue = ui8_modulPortType;
        SetDisplayPanelModeSVModulPortType();  // mode = 78
      }
      else if(b_Edit)
      {
        boolean b_Display(false);
        if (ui8_buttons & BUTTON_KEYPAD)
          b_Display = true;
        if (ui8_buttons & BUTTON_UP)
        { 
          // Wert verkleinern:
          --ui16_EditValue;
          if(ui16_EditValue < 1)
            ui16_EditValue = 1;
          b_Display = true;
        }
        else if (ui8_buttons & BUTTON_DOWN)
        { 
          // Wert vergrößern:
          ++ui16_EditValue;
          if(ui16_EditValue > MAXPORTADR)
            ui16_EditValue = MAXPORTADR;
          b_Display = true;
        }
        if(b_Display)
        {
          displayValue4(ui16_EditValue);
        }
      }
      return;
    } // if(ui8_DisplayPanelMode == 77)
    //------------------------------------
    if(ui8_DisplayPanelMode == 78)
    {
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to ask for new port-address
        ui16_EditValue = ui16_modulPortAddress;
        SetDisplayPanelModeSVModulPortAddress();  // mode = 77
      }
      else if (ui8_buttons & BUTTON_SELECT)
      { // switch to ask for send to Port
        ui8_modulPortType = (uint8_t)(ui16_EditValue & 0xFF);
        if(ui8_modulPortType < getMaxPortType())
          SetDisplayPanelModeRequestSendToPort(); // mode = 79
      }
      else if(b_Edit)
      {
        boolean b_Display(false);
        if (ui8_buttons & BUTTON_UP)
        { 
          // Wert verkleinern:
          --ui16_EditValue;
          if((uint8_t)(ui16_EditValue & 0xFF) == 255) // 0 -> 255
            ui16_EditValue = 0;
          b_Display = true;
        }
        else if (ui8_buttons & BUTTON_DOWN)
        { 
          // Wert vergrößern:
          ++ui16_EditValue;
          if(ui16_EditValue >= getMaxPortType())
            ui16_EditValue = getMaxPortType() - 1;
          b_Display = true;
        }
        if(b_Display)
        {
          displayPortTypeAsText((uint8_t)(ui16_EditValue & 0xFF));
        }
      }
      return;          
    } // if(ui8_DisplayPanelMode == 78)
    //------------------------------------
    if(ui8_DisplayPanelMode == 79)
    {
      if (ui8_buttons & BUTTON_SELECT)
      { // send data to Port:
        WriteOnePortToModule(ui8_modulAddress, ui8_modulSubAddress, ui8_modulPortNumber, ui16_modulPortAddress, ui8_modulPortType);
        ui8_DisplayPanelMode = 80;
        SetDisplayPanelUebertragung();
      }
      else if (ui8_buttons)  // any other button returns
      { // switch to ask for modul-Port
        SetDisplayPanelModeSVModulPortnumber(); // mode = 76
      }
      return;
    } // if(ui8_DisplayPanelMode == 79)
    //------------------------------------
    if(ui8_DisplayPanelMode == 81)
    {
      if (ui8_buttons)  // any button returns
      { // switch to ask for modul-Port
        SetDisplayPanelModeSVModulPortnumber(); // mode = 76
      }
      return;
    } // if(ui8_DisplayPanelMode == 81)
    //------------------------------------
    if(ui8_DisplayPanelMode == 90)
    {
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to ask for modul-Port
        SetDisplayPanelModeSVModulPortnumber(); // mode = 76
      }
      else if (ui8_buttons & BUTTON_SELECT)
      { 
        SetDisplayPanelModePICAlone();  // mode = 91
      }
      return;
    } // if(ui8_DisplayPanelMode == 90)
    //------------------------------------
    if(ui8_DisplayPanelMode == 91)
    {
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to ask for modul-Port
        SetDisplayPanelModeSVModulPortnumber(); // mode = 76
      }
      else if (ui8_buttons & BUTTON_SELECT)
      { 
        SetDisplayPanelInitialisierung(); // mode = 92
        InitModul();
      }
      return;
    } // if(ui8_DisplayPanelMode == 91)
    //------------------------------------
    if(ui8_DisplayPanelMode == 100)
    {
      boolean b_Display(false);
      // actual ui8_DisplayPanelMode = show IO-Telegrams
      if (ui8_buttons & BUTTON_DOWN)
      { 
        b_Display = true;
        ++ui16_EditValue;
        if(ui16_EditValue >= GetModulMaxData())
          ui16_EditValue = 0;
      }
      else if (ui8_buttons & BUTTON_UP)
      { 
        b_Display = true;
        if(ui16_EditValue == 0)
          ui16_EditValue = GetModulMaxData();
        --ui16_EditValue;
      }
      else if (ui8_buttons & BUTTON_LEFT)
      { // switch to Beobachten?
        SetWaitForTelegram(0);
        SetDisplayPanelModeWatchIOTelegram();  // mode = 6
        return;
      }
      if(b_Display)
        DisplayTelegrams();
      return;
    } // if(ui8_DisplayPanelMode == 100)
    //------------------------------------
    if(ui8_DisplayPanelMode == 110)
    {
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to Control
        SetDisplayPanelModeControl();  // mode = 7
      }
      else if (ui8_buttons & BUTTON_SELECT)
      { // switch to ask for new port-type
        ui16_SwitchAddress = ui16_EditValue;
        SetDisplayPanelModeTelegramType();  // mode = 111
      }
      else if(b_Edit)
      {
        boolean b_Display(false);
        if (ui8_buttons & BUTTON_KEYPAD)
          b_Display = true;
        if (ui8_buttons & BUTTON_UP)
        { 
          // Wert verkleinern:
          --ui16_EditValue;
          if(ui16_EditValue < 1)
            ui16_EditValue = 1;
          b_Display = true;
        }
        else if (ui8_buttons & BUTTON_DOWN)
        { 
          // Wert vergrößern:
          ++ui16_EditValue;
          if(ui16_EditValue > MAXPORTADR)
            ui16_EditValue = MAXPORTADR;
          b_Display = true;
        }
        if(b_Display)
        {
          displayValue4(ui16_EditValue);
        }
      }
      return;
    } // if(ui8_DisplayPanelMode == 110)
    //------------------------------------
    if(ui8_DisplayPanelMode == 111)
    {
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to Address
        SetDisplayPanelModePortAddress();  // mode = 110
      }
      else if (ui8_buttons & BUTTON_SELECT)
      { // switch to ask for telegramType
        ui8_TelegramKind = (ui16_EditValue & 0xFF);
        SetDisplayPanelModeTelegramState();  // mode = 112
      }
      else if(b_Edit)
      {
        boolean b_Display(false);
        if (ui8_buttons & BUTTON_KEYPAD)
          b_Display = true;
        if (ui8_buttons & BUTTON_UP)
        { 
          // Wert verkleinern:
          if(ui16_EditValue > 0)
            --ui16_EditValue;
          b_Display = true;
        }
        else if (ui8_buttons & BUTTON_DOWN)
        { 
          // Wert vergrößern:
          if(ui16_EditValue < 2)
            ++ui16_EditValue;
          b_Display = true;
        }
        if(b_Display)
        {
          displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
          displayPanel.print('>');
          displayPanel.print('B');
          displayPanel.print(ui16_EditValue);
        }
      }
      return;
    } // if(ui8_DisplayPanelMode == 111)
    //------------------------------------
    if(ui8_DisplayPanelMode == 112)
    {
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to Control
        SetDisplayPanelModeTelegramType();  // mode = 111
      }
      else if (ui8_buttons & BUTTON_SELECT)
      { // send Telegram:
        LocoNetKS.sendSwitchState(ui16_SwitchAddress, true, b_SwitchState, OPC_SW_REQ + ui8_TelegramKind);
        if( !ui8_TelegramKind && (b_SwitchState) && B0_OFF)
        { // with B0 we may send shortly after on an off
          if(GetCV(ID_DELAY_B0))
          { // delay in 10ms
            ui8_DisplayPanelMode = 113;
            ul_Wait = millis();
            return;
          }
        }
        // switch to Address
        SetDisplayPanelModePortAddress();  // mode = 110
      }
      else if(b_Edit)
      {
        boolean b_Display(false);
        if (ui8_buttons & BUTTON_KEYPAD)
          b_Display = true;
        if (ui8_buttons & BUTTON_UP)
        { 
          // Wert verkleinern:
          b_SwitchState = false;
          b_Display = true;
        }
        else if (ui8_buttons & BUTTON_DOWN)
        { 
          // Wert vergrößern:
          b_SwitchState = true;
          b_Display = true;
        }
        if(b_Display)
        {
          displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
          displayPanel.print('>');
          displayPanel.print(b_SwitchState ? F("ein"): F("aus"));
        }
      }
      return;
    } // if(ui8_DisplayPanelMode == 112)
    //------------------------------------
    if(ui8_DisplayPanelMode == 200)
    {
      // actual ui8_DisplayPanelMode = "CV?"
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to I²C-Scan
        SetDisplayPanelModeScan();
      }
      else if (ui8_buttons & BUTTON_LEFT)
      { // switch to "IBN?"
        SetDisplayPanelModeIBN();
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to display current CV's
        ui8_DisplayPanelMode = 20;
        DisplayCV(GetCV(ID_DEVICE));
      }
      return;
    } // if(ui8_DisplayPanelMode == 200)
    //------------------------------------
    if(ui8_DisplayPanelMode == 210)
    {
      // actual ui8_DisplayPanelMode = "I²C-Scan?"
      if (ui8_buttons & BUTTON_UP)
      { // switch to "CV?"
        SetDisplayPanelModeCV();
      }
#if defined ENABLE_KEYPAD
      else if (ui8_buttons & BUTTON_DOWN)
      { // switch to "Tastatur-Test"
        if(isKeypadPresent())
        {
          SetDisplayPanelModeKeypadTest();
        }
      }
#endif
      else if (ui8_buttons & BUTTON_LEFT)
      { // switch to "Inbetriebnahme?"
        SetDisplayPanelModeIBN();
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to display I²C-Addresses
        ui8_DisplayPanelMode = 211;
        InitScan();
      }
#if defined ETHERNET_BOARD
      else if (ui8_buttons & BUTTON_DOWN)
      { // switch to display IP-Address
        SetDisplayPanelModeIpAdr();
      }
#endif
      return;
    } // if(ui8_DisplayPanelMode == 210)
    //------------------------------------
    if(ui8_DisplayPanelMode == 211)
    {
      // actual ui8_DisplayPanelMode = "I²C-Scan"
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to "I²C-Scan?"
        SetDisplayPanelModeScan();
      }
      else if (ui8_buttons & BUTTON_DOWN)
      {
        NextScan();
      }
      return;
    } // if(ui8_DisplayPanelMode == 211)
    //------------------------------------
#if defined ENABLE_KEYPAD
    if(ui8_DisplayPanelMode == 215)
    {
      // actual ui8_DisplayPanelMode = "Tastatur-Test"
      if (ui8_buttons & BUTTON_UP)
      { // switch to I²C-Scan
        SetDisplayPanelModeScan();
      }
      return;
    }
#endif
    //------------------------------------
#if defined ETHERNET_BOARD
    if(ui8_DisplayPanelMode == 220)
    {
      // actual ui8_DisplayPanelMode = "IP-Address"
      if (ui8_buttons & BUTTON_UP)
      { // switch to "I²C-Scan ?"
        SetDisplayPanelModeScan();
      }
      else if (ui8_buttons & BUTTON_DOWN)
      { // switch to "MAC-Address"
        SetDisplayPanelModeMacAdr();
      }
      return;
    } // if(ui8_DisplayPanelMode == 220)
    //------------------------------------
    if(ui8_DisplayPanelMode == 221)
    {
      // actual ui8_DisplayPanelMode = "MAC-Address"
      if (ui8_buttons & BUTTON_FCT_BACK)
      { // any key returns to "IP-Address"
        SetDisplayPanelModeIpAdr();
      }
      return;
    } // if(ui8_DisplayPanelMode == 221)
#endif
  } // if(ui8_buttons && (ui8_ButtonMirror != ui8_buttons))
  //========================================================================
  if(ui8_DisplayPanelMode == 5)
  {
    uint16_t i = GetStoerung();
    if(ui16_StoerungMirror != i)
    {
      ui16_StoerungMirror = i;
      displayPanel.setCursor(0, 1);  // set the cursor to column x, line y
      binout(displayPanel, i >> 8);
      binout(displayPanel, i & 0x00FF);
    }
  } // if(ui8_DisplayPanelMode == 5)
  else
    ui16_StoerungMirror = 65535;
  //------------------------------------
  if(ui8_DisplayPanelMode == 80)
  {
    if(GetStoerung())
    {
      SetDisplayPanelStoerungBeiUebertragung();
    }
    else if (!GetWaitForTelegram())
    { // switch to ask for modul-Port
      SetDisplayPanelModeSVModulPortnumber(); // mode = 76
    }
    return;
  } // if(ui8_DisplayPanelMode == 80)
  //------------------------------------
  if(ui8_DisplayPanelMode == 82)
  {
    if(GetStoerung())
    {
      SetDisplayPanelStoerungBeiUebertragung();
    }
    else if (!GetWaitForTelegram())
    {
      SetDisplayPanelModeSVModulPortnumber(); // mode = 76
    }
    return;
  } // if(ui8_DisplayPanelMode == 82)
  //------------------------------------
  if(ui8_DisplayPanelMode == 83)
  {
    if((millis() - ul_Wait) > 1000)
    {
      ul_Wait = millis();
      displayPanel.setCursor(ui16_EditValue++, 1);
      displayPanel.print('.');
    }
    if(ReadAllModulAddressesFinished())
    {
      ui8_DisplayPanelMode = 73;
      ui16_EditValue = -1;
      NextModul();
    }
    return;
  } // if(ui8_DisplayPanelMode == 83)
  //------------------------------------
  if(ui8_DisplayPanelMode == 92)
  {
    if(GetStoerung())
    {
      SetDisplayPanelStoerungBeiUebertragung();
    }
    else if (!GetWaitForTelegram())
    { // switch to ask for modul-Port
      SetDisplayPanelUebertragung();  // mode = 82
      ui16_EditValue = 1;
			ui8_modulPortNumber = 1;
      ReadOnePortOfModule(ui8_modulAddress, ui8_modulSubAddress, ui8_modulPortNumber);
    }
    return;
  } // if(ui8_DisplayPanelMode == 92)
  //------------------------------------
  if(ui8_DisplayPanelMode == 100)
  {
    if(HasNewTelegram())
      DisplayTelegrams();
    return;
  } // if(ui8_DisplayPanelMode == 100)
  //------------------------------------
  if(ui8_DisplayPanelMode == 113)
  {
    if ((millis() - ul_Wait) > (GetCV(ID_DELAY_B0) * 10))
    { // B0 off:
      LocoNetKS.sendSwitchState(ui16_SwitchAddress, true, false, OPC_SW_REQ);   // Telegramm B0
      // switch to Address
      SetDisplayPanelModePortAddress();  // mode = 110
    }
  } // if(ui8_DisplayPanelMode == 113)
  //------------------------------------
#if defined ENABLE_KEYPAD
  if(ui8_DisplayPanelMode == 215)
  {
    uint8_t chKey;
    if(getKey(&chKey))
    {
      displayPanel.setCursor(0, 1);
      displayPanel.print(char(chKey));
    }
    if((digitalRead(CTRL_BUTTON) == LOW) && !b_CtrlMirror)
    {
      b_CtrlMirror = true;
      displayPanel.setCursor(3, 1);
      displayPanel.print(F("Ctrl"));
    }
    if((digitalRead(CTRL_BUTTON) == HIGH) && b_CtrlMirror)
    {
      b_CtrlMirror = false;
      displayPanel.setCursor(3, 1);
      displayPanel.print(F("    "));
    }
  } // if(ui8_DisplayPanelMode == 215)
#endif
  //====================================
}

#endif // defined LCD
