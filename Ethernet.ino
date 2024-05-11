//=== Ethernet === usable for all ====================================
#if defined ETHERNET_BOARD
#include <SPI.h>
#include <Ethernet.h>
#include <LocoNetKS.h>

#define CS_SD_CARD 4

//=== declaration of var's =======================================
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress IP(192, 168, 255, 255);

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

//=== functions ==================================================
const __FlashStringHelper *GetLocoIOTitle() { return F("LocoIO-Support"); }
uint8_t *GetMACAddress() { return mac; }

void InitEthernet()
{
#if not defined USE_SD_CARD_ON_ETH_BOARD
  // disable the SD card by switching pin 4 high
  // not using the SD card in this program, but if an SD card is left in the socket,
  // it may cause a problem with accessing the Ethernet chip, unless disabled
  pinMode(CS_SD_CARD, OUTPUT);
  digitalWrite(CS_SD_CARD, HIGH);
#endif

  // start the Ethernet connection and the server:
  IP[2] = GetCV(IP_BLOCK_3);
  IP[3] = GetCV(IP_BLOCK_4);
  Ethernet.begin(mac, IP);
  server.begin();
#if defined DEBUG
  Serial.print(F("Ethernet-Server at "));
  Serial.println(Ethernet.localIP());
#endif
}

void HandleEthernetRequests()
{
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client)
  {
#if defined DEBUG
    Serial.println(F("new client"));
#endif
    // an http request ends with a blank line
    boolean b_currentLineIsBlank(true);
    uint8_t ui8_iReceivedCharStep = 0;
    uint8_t ui8_iReceivedCharStepIO = 0;
    uint8_t ui8_iReceivedCharStepIOhtm = 0;
    uint8_t ui8_iReceivedCharStepClock = 0;
    uint16_t ui16_ReceivedCV = 0;  // ui16_ReceivedCV is real CV-Address and begins with 1
    uint16_t ui16_ReceivedValue = 0;  
    while (client.connected())
    {
      if (client.available())
      {
        /*
         * pressing a button "save" (available for each cv) will generate a http-request as follows:
         * GET /?cv3=15 HTTP/1.1 <lf> <some more data...>
         *    with '3' as cv-number and '15' as cv-value
         * 
         * GET /io.htm HTTP/1.1 <lf> <some more data...>
         * 
         * GET /?io618=3 HTTP/1.1 <lf> <some more data...>
         *    with '618' as LocoIO-Address and '3' as IO-State (see below...)
         * 
         * GET /io.htm?io618=3 HTTP/1.1 <lf> <some more data...>
         *    with '618' as LocoIO-Address and '3' as IO-State (see below...)
         * 
         * GET /?mod=0 HTTP/1.1 <lf> <some more data...>
         *    with '0' => stop clock
         *    with '1' => start clock
         * 
         */
        char c(client.read());
#if defined DEBUG
        Serial.write(c);
#endif
        switch (ui8_iReceivedCharStep)
        {
          case 0 : if(c == '?')
                   {
                     ui8_iReceivedCharStepClock = 0;
                     ui8_iReceivedCharStepIOhtm = 0;
                     ++ui8_iReceivedCharStep;
                   }
                   else
                     if((c == 'i') || (c == 'I'))
                     {
                       ++ui8_iReceivedCharStep;
                       ++ui8_iReceivedCharStepIOhtm;
                     }
                     else
                       if((c == 'm') || (c == 'M'))
                         ui8_iReceivedCharStep = 101;
                   break;
          case 1 : if((c == 'c') || (c == 'C'))
                     ++ui8_iReceivedCharStep;
                   else
                     if((c == 'i') || (c == 'I'))
                     {
                       ++ui8_iReceivedCharStep;
                       ++ui8_iReceivedCharStepIO;
                     }
                     else
                       if((c == 'o') || (c == 'O'))
                       {
                         ++ui8_iReceivedCharStep;
                         ++ui8_iReceivedCharStepIOhtm;
                       }
                       else
                         if((c == 'm') || (c == 'M'))
                         {
                           ++ui8_iReceivedCharStep;
                           ++ui8_iReceivedCharStepClock;
                         }
                         else
                         {
                           ui8_iReceivedCharStepClock = 0;
                           ui8_iReceivedCharStep = 0;
                           ui8_iReceivedCharStepIO = 0;
                           ui8_iReceivedCharStepIOhtm = 0;
                         }
                   break;
          case 2 : if((c == 'v') || (c == 'V'))
                     ++ui8_iReceivedCharStep;
                   else
                     if((c == 'o') || (c == 'O'))
                     {
                       ++ui8_iReceivedCharStep;
                       ++ui8_iReceivedCharStepIO;
                       ++ui8_iReceivedCharStepClock;
                     }
                     else
                       if(c == '.')
                       {
                         ++ui8_iReceivedCharStep;
                         ++ui8_iReceivedCharStepIOhtm;
                       }
                       else
                       {
                         ui8_iReceivedCharStepClock = 0;
                         ui8_iReceivedCharStep = 0;
                         ui8_iReceivedCharStepIO = 0;
                         ui8_iReceivedCharStepIOhtm = 0;
                       }
                   break;
          case 3 : if(c == '=')
                     ++ui8_iReceivedCharStep;
                   else
                   {
                     if((c == 'd') || (c == 'D'))
                     {
                       ++ui8_iReceivedCharStep;
                       ++ui8_iReceivedCharStepClock;
                     }
                     else
                       if((c == 'h') || (c == 'H'))
                       {
                         ++ui8_iReceivedCharStep;
                         ++ui8_iReceivedCharStepIOhtm;
                       }
                       else
                       {
                         if((c >= '0') && (c <= '9'))
                           ui16_ReceivedCV = 10 * ui16_ReceivedCV + (c - '0');
                       }
                   }
                   break;
          case 4 : if((c == '=') && (ui8_iReceivedCharStepClock == 3))
                   {
                     ++ui8_iReceivedCharStep;
                     ++ui8_iReceivedCharStepClock;
                   }
                   else
                     if((c >= '0') && (c <= '9'))
                       ui16_ReceivedValue = 10 * ui16_ReceivedValue + (c - '0');
                     else
                     {
                       if((c == 't') || (c == 'T'))
                         ++ui8_iReceivedCharStepIOhtm;
                       ++ui8_iReceivedCharStep;
                     }
                   break;
          case 5 : if((ui8_iReceivedCharStepClock == 4) && ((c == '0') || (c == '1')))
                   {
#if defined ETHERNET_CLOCKCOMMANDER
                     ui16_ReceivedValue = (c - '0');
                     SetClockMode(ui16_ReceivedValue);
#if defined DEBUG
                     Serial.println();
                     Serial.print(F("=> new Clock-mode:"));
                     Serial.println(ui16_ReceivedValue);
#endif                   
#endif
                   }
                   else
                     if((c == ' ') || (c == '/'))
                       ui8_iReceivedCharStep = 100;
                     else
                     {
                       if((c == 'm') || (c == 'M'))
                       {
                         ++ui8_iReceivedCharStep;
                         ++ui8_iReceivedCharStepIOhtm;
                       }
                     }
                   break;
          case 6 : if(c == '?')
                     ui8_iReceivedCharStep = 1;
                   break;
          case 100 : if ((c == '\n') && b_currentLineIsBlank)
                     {
                       if((ui8_iReceivedCharStepIOhtm == 0) && (ui8_iReceivedCharStepIO == 0))
                       {
#if defined DEBUG
                         Serial.print(F("CV"));
                         Serial.print(ui16_ReceivedCV);
                         Serial.print('=');
                         Serial.println(ui16_ReceivedValue);
#endif                   
#if defined ETHERNET_CLOCKCOMMANDER
                         if(ui16_ReceivedCV == 2) //ID_DEVIDER
                           SetDevider((uint8_t)(ui16_ReceivedValue & 0xFF));
                         else 
#endif
                         CheckAndWriteCVtoEEPROM((--ui16_ReceivedCV) & 0xFF, ui16_ReceivedValue);
                       }
#if defined ETHERNET_WITH_LOCOIO
                       if(ui8_iReceivedCharStepIO == 2)
                       {
                         /*  ui16_ReceivedValue (bin-coded): .... dttv
                          *  d = direction       : 0=red/straight, 1=green/round
                          *  t = type of telegram: 00=B0, 01=B1, 10=B2
                          *  v = value           : 0=off, 1=on
                          */
#if defined DEBUG
                         Serial.print(F("IO"));
                         Serial.print(ui16_ReceivedCV);
                         Serial.print('=');
                         Serial.println(ui16_ReceivedValue);
#endif                        
                         LocoNetKS.sendSwitchState(ui16_ReceivedCV, (ui16_ReceivedValue & 0x01), (ui16_ReceivedValue & 0x08), OPC_SW_REQ + ((ui16_ReceivedValue & 0x06) >> 1));
                       }
#endif  // ETHERNET_WITH_LOCOIO
                     }
                   break;
        }
#if defined ETHERNET_PAGE_SERVER
#if not defined ETHERNET_WITH_LOCOIO
        ui8_iReceivedCharStepIOhtm = 0;
#endif

        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && b_currentLineIsBlank)
        {
          // send a standard http response header
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/html"));
          client.println(F("Connection: close"));  // the connection will be closed after completion of the response
          client.println();

          // send head of each page
          client.println(F("<!DOCTYPE HTML>"));
          client.print(F("<html><head><title>"));
          client.print(GetSwTitle());
          client.print(F("</title>"));
#if defined ETHERNET_WITH_LOCOIO
          if(ui8_iReceivedCharStepIOhtm == 6)
          {
            // for scripting: output each line with 'println' !
            client.println(F("<script language=\"JavaScript\"><!--"));
            client.println(F("function senden(obj) { var status;"));
            client.println(F("for (var i=0; i<document.inp.status.length; i++) if (document.inp.status[i].checked) status=document.inp.status[i].value;"));
            client.println(F("for (var i=0; i<document.inp.dir.length; i++) if (document.inp.dir[i].checked) status=String(Number(document.inp.dir[i].value)+Number(status));"));
            client.println(F("obj.href+=\"?io\"+document.inp.io.value+\"=\"+status;}"));
            client.println(F("--></script>"));
          }
#endif  // ETHERNET_WITH_LOCOIO
          client.print(F("</head><body><h1>"));
          client.print(GetSwTitle());
          client.print(F("</h1><hr><h2>"));

#if defined ETHERNET_CLOCKCOMMANDER
		      if(IsClockRunning())
			      client.print(F("Running<form method=get><button name=mod value=0> Uhren anhalten</button></form>"));
          else
			      client.print(F("Stopped<form method=get><button name=mod value=1> Uhren starten</button></form>"));
          client.print(F("<hr>"));
#endif //ETHERNET_CLOCKCOMMANDER

          if(ui8_iReceivedCharStepIOhtm != 6)
          { // show first(index) page
            client.println(F("Current CV-settings</h2><table>"));
            // output the value of each cv
            for(uint8_t i = 0; i < GetCVCount(); i++)
            {
              client.print(F("<tr><td>"));
              boolean b_Form(CanEditCV(i));
              if(b_Form)
                client.print(F("<form method=get>"));
              client.print(F("CV "));
              client.print(i + 1);
              client.print(F(" ("));
              client.print(GetCVName(i));
              client.print(F("): "));
              client.print(F("</td><td>"));
              if(b_Form)
              {
                client.print(F("<input name=cv"));
                client.print(i + 1);
                client.print(F(" size=5 maxlength=3 value="));
              }
              client.print(GetCV(i));
              if(b_Form)
                client.print(F("><input type=submit value=save /></form>")); // generates he button "save"
              else
                client.println(F("<br/>"));
              client.print(F("</td></tr>"));
            } // for(uint8_t i = 0; i < GetCVCount(); i++)
            client.println(F("</table>"));
            client.println(F("<form method=get><button name=cv7 value=0 >alle CV auf Standard setzen</button></form><br>"));
#if defined ETHERNET_WITH_LOCOIO
            client.print(F("<hr><br/><a href=\"io.htm\">"));
            client.print(GetLocoIOTitle());
            client.print(F("</a>"));
#endif  // ETHERNET_WITH_LOCOIO
          } // show first(index) page
#if defined ETHERNET_WITH_LOCOIO
          else
          { // LocoIO-support page
            client.print(GetLocoIOTitle());
            client.print(F("</h2>"));

            client.print(F("<form name=inp>"));
            client.print(F("LocoIO-Address: <input name=io size=5 maxlength=4 value=1 /><br/>"));
            client.print(F("B0: <input type=radio name=status value=0 checked/>off <input type=radio name=status value=1 />on<br/>"));
            client.print(F("B1: <input type=radio name=status value=2 />off <input type=radio name=status value=3 />on<br/>"));
            client.print(F("B2: <input type=radio name=status value=4 />off <input type=radio name=status value=5 />on<br/>"));
            client.print(F("Direction: <input type=radio name=dir value=0 checked/>red/straight <input type=radio name=dir value=8 />green/round<br/>"));
            client.print(F("<a href=\"io.htm\" onclick=\"senden(this);\">Send</a></form>"));

            client.print(F("<hr><br/><a href=\"main.htm\">back to main page</a>"));
            ui8_iReceivedCharStepIOhtm = 0;
          } // LocoIO-support page
          ui8_iReceivedCharStepIOhtm = 0;
#endif  // ETHERNET_WITH_LOCOIO
          client.print(F("<hr>active since "));
          unsigned long ulTimeSinceStart = (millis() / 1000); // ulTimeSinceStart is in seconds
          if(ulTimeSinceStart < 60)
          {
            client.print(ulTimeSinceStart);
            client.print('s');
          }
          else
          {
            client.print(ulTimeSinceStart / 60);
            client.print(':');
            uint16_t l = ulTimeSinceStart % 60;
            if(l < 10)
              client.print('0');
            client.print(l);
            client.print(F("min"));
          }
          client.println(F("</body></html>"));
          break; // get out of 'while' here
        } // if (c == '\n' && currentLineIsBlank)
#endif // ETHERNET_PAGE_SERVER
        if (c == '\n')
        {
          // you're starting a new line
          b_currentLineIsBlank = true;
        } 
        else if (c != '\r')
        {
          // you've gotten a character on the current line
          b_currentLineIsBlank = false;
        }
      } // if (client.available())
    } // while (client.connected())
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
#if defined DEBUG
    Serial.println(F("client disconnected"));
#endif
  } // if (client)
}

#endif
