//=== CV-Definitions for LocoIO ===

// give CV a unique name
enum {  ID_DEVICE = 0, ID_OWN, ID_DELAY_B0, ID_UNUSED_2, ID_UNUSED_3, ID_UNUSED_4, VERSION_NUMBER, SOFTWARE_ID, ADD_FUNCTIONS_1
#if defined ETHERNET_BOARD
      , IP_BLOCK_3, IP_BLOCK_4
#endif
};

//=== declaration of var's =======================================
#define PRODUCT_ID SOFTWARE_ID
#define ADR_MAIN_CONTROLLER   80

static const uint8_t DEVICE_ID = ADR_MAIN_CONTROLLER;   // CV1: Device-ID
static const uint8_t SW_VERSION = 8;										// CV7: Software-Version
static const uint8_t LOCOIOEDIT = 6;										// CV8: Software-ID

#if defined ETHERNET_BOARD
  static const uint8_t MAX_CV = 11;
#else
  static const uint8_t MAX_CV = 9;
#endif

uint16_t ui16a_CV[MAX_CV] = { UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX
#if defined ETHERNET_BOARD
                            , MAX_VALUE, MAX_VALUE
#endif
                          };  // ui16a_CV is a copy from eeprom

struct _CV_DEF // uses 9 Byte of RAM for each CV
{
  uint8_t ID;
  uint16_t DEFLT;
  uint16_t MIN;
  uint16_t MAX;
  uint8_t TYPE;
  boolean RO;
};

enum CV_TYPE { UI8 = 0, UI16 = 1, BINARY = 2 };

const struct _CV_DEF cvDefinition[MAX_CV] =
{ // ID									default value        minValue							maxValue							type              r/o
	 { ID_DEVICE,					DEVICE_ID,           1,										126,				          CV_TYPE::UI8,     false}  // normally r/o
	,{ ID_OWN,						DEVICE_ID,  				 1,										126,							    CV_TYPE::UI8,     false}  // my own ID
	,{ ID_DELAY_B0,				10,                  0,										UINT8_MAX,				    CV_TYPE::UI8,     false}  // delay for sending B0
	,{ ID_UNUSED_2,				0,                   0,										0,										CV_TYPE::UI8,     true}   // (not used)
	,{ ID_UNUSED_3,				0,                   0,										0,										CV_TYPE::UI8,     true}   // (not used)
	,{ ID_UNUSED_4,				0,                   0,										0,										CV_TYPE::UI8,     true}   // (not used)
	,{ VERSION_NUMBER,		SW_VERSION,          0,										SW_VERSION,						CV_TYPE::UI8,     false}  // normally r/o
	,{ SOFTWARE_ID,				LOCOIOEDIT,					 LOCOIOEDIT,					LOCOIOEDIT,						CV_TYPE::UI8,     true}   // always r/o
	,{ ADD_FUNCTIONS_1,		0x00,                0,										UINT8_MAX,						CV_TYPE::BINARY,  false}  // additional functions 1
#if defined ETHERNET_BOARD
	,{ IP_BLOCK_3,				2,                   0,										UINT8_MAX,						CV_TYPE::UI8,     false}  // IP-Address part 3
	,{ IP_BLOCK_4,				106,                 0,										UINT8_MAX,						CV_TYPE::UI8,     false}  // IP-Address part 4
#endif
};


//=== naming ==================================================
const __FlashStringHelper* GetSwTitle() { return F("LocoIO-SV-Editor"); }
//========================================================
const __FlashStringHelper *GetCVName(uint8_t ui8_Index)
{ 
  // each string should have max. 10 chars
  const __FlashStringHelper *cvName[MAX_CV] = { F("DeviceID"),
                                                F("OwnID"),
                                                F("Delay B0"),
                                                F("not used"),
                                                F("not used"),
                                                F("not used"),
                                                F("Version"),
                                                F("SW-ID"),
                                                F("Config 1")
#if defined ETHERNET_BOARD
                                              , F("IP-Part 3"), F("IP-Part 4")
#endif
                                                };
                                    
  if(ui8_Index < MAX_CV)
    return cvName[ui8_Index];
  return F("???");
}

//=== functions ==================================================
boolean AlreadyCVInitialized() { return (ui16a_CV[SOFTWARE_ID] == LOCOIOEDIT); }
