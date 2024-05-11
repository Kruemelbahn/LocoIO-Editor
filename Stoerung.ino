//=== Stoerung for LocoIO ===

//=== declaration of var's =======================================
uint16_t ui16_Stoerung = 0;
/*
 * Bit  0 : Fehler beim Schreiben SV <port*3>
 * Bit  1 : Fehler beim Schreiben SV <port*3> + 1
 * Bit  2 : Fehler beim Schreiben SV <port*3> + 2
 * Bit  3 : Fehler beim Lesen SV <port*3>, SV <port*3> + 1, SV <port*3> + 2
 * 
 * 
 * 
 * 
 * 
 * Bit  8 : Zeitüberlauf beim Schreiben SV <port*3>
 * Bit  9 : Zeitüberlauf beim Schreiben SV <port*3> + 1
 * Bit 10 : Zeitüberlauf beim Schreiben SV <port*3> + 2
 * Bit 11 : Zeitüberlauf beim Lesen SV <port*3>, SV <port*3> + 1, SV <port*3> + 2
 * 
 * 
 * 
 * 
 */
 
uint16_t GetStoerung() { return ui16_Stoerung; }
void     ResetStoerung() { ui16_Stoerung = 0; }
void     SetStoerung(uint16_t ui16_Disturbance) { ui16_Stoerung |= ui16_Disturbance; }

