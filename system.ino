//=== system === usable for all ====================================
static const uint16_t INTERVAL_125MS = 125;
unsigned long ul_previousMillis = 0;
boolean b_Blinken05Hz = false;
boolean b_Blinken1Hz = false;
boolean b_Blinken2Hz = false;
boolean b_Blinken4Hz = false;

void Blinken()
{
  if (millis() - ul_previousMillis > INTERVAL_125MS)
  {
    ul_previousMillis = millis();   // keep actual timevalue
    // change status.
    b_Blinken4Hz = !b_Blinken4Hz;
    if(b_Blinken4Hz)
    {
      b_Blinken2Hz = !b_Blinken2Hz;
      if(b_Blinken2Hz)
      {
        b_Blinken1Hz = !b_Blinken1Hz;
        if(b_Blinken1Hz)
        {
          b_Blinken05Hz = !b_Blinken05Hz;
        }
      }
    }
  }
}

boolean Blinken05Hz() { return b_Blinken05Hz; }
boolean Blinken1Hz() { return b_Blinken1Hz; }
boolean Blinken2Hz() { return b_Blinken2Hz; }
boolean Blinken4Hz() { return b_Blinken4Hz; }

#if defined DEBUG
  #if defined DEBUG_MEM
    #include <MemoryFree.h>
    
    void ViewFreeMemory()
    {
        Serial.print(F("freeMemory()="));
        Serial.println(freeMemory());
    }
    
    /* ==============================================================
     * This function places the current value of the heap and stack pointers in the
     * variables. You can call it from any place in your code and save the data for
     * outputting or displaying later. This allows you to check at different parts of
     * your program flow.
     * The stack pointer starts at the top of RAM and grows downwards. The heap pointer
     * starts just above the static variables etc. and grows upwards. SP should always
     * be larger than HP or you'll be in big trouble! The smaller the gap, the more
     * careful you need to be. Julian Gall 6-Feb-2009.
     */
    uint8_t *ui8_heapptr, *ui8_stackptr;
    void Check_mem()
    {
      ui8_stackptr = (uint8_t *)malloc(4); // use stackptr temporarily
      ui8_heapptr = ui8_stackptr;          // save value of heap pointer
      free(ui8_stackptr);                  // free up the memory again (sets stackptr to 0)
      ui8_stackptr = (uint8_t *)(SP);      // save value of stack pointer
    }
    
    //==============================================================
    unsigned long ul_TimeStart = 0;
    void ShowTimeDiff()
    {
      Serial.print(F("LoopTime="));
      if(ul_TimeStart > 0)
        Serial.print(millis() - ul_TimeStart);
      ul_TimeStart = millis();
      Serial.println(F("ms"));
    }
    //==============================================================
  #endif
#endif
