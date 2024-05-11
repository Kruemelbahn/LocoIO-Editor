//=== GlobalOutPrint === usable for all ====================================
#if defined LCD

void binout16(Print &stream, uint16_t ui16_Out)
{
  if(ui16_Out<32768)
    stream.print('0');
  if(ui16_Out<16384)
    stream.print('0');
  if(ui16_Out<8192)
    stream.print('0');
  if(ui16_Out<4096)
    stream.print('0');
  if(ui16_Out<2048)
    stream.print('0');
  if(ui16_Out<1024)
    stream.print('0');
  if(ui16_Out<512)
    stream.print('0');
  stream.print(ui16_Out >> 8, BIN);
	binout(stream, (uint8_t)(ui16_Out & 0x00FF));
}

void binout(Print &stream, uint8_t ui8_Out)
{
  if(ui8_Out<128)
    stream.print('0');
  if(ui8_Out<64)
    stream.print('0');
  if(ui8_Out<32)
    stream.print('0');
  if(ui8_Out<16)
    stream.print('0');
  if(ui8_Out<8)
    stream.print('0');
  if(ui8_Out<4)
    stream.print('0');
  if(ui8_Out<2)
    stream.print('0');
  stream.print(ui8_Out, BIN);
}

void decout(Print& stream, uint16_t ui16_Out, uint8_t ui8CountOfDigits)
{
  // 5 digits, 0..99999
  if ((ui8CountOfDigits > 4) && (ui16_Out < 10000))
    stream.print('0');
  if ((ui8CountOfDigits > 3) && (ui16_Out < 1000))
    stream.print('0');
  if ((ui8CountOfDigits > 2) && (ui16_Out < 100))
    stream.print('0');
  if ((ui8CountOfDigits > 1) && (ui16_Out < 10))
    stream.print('0');
  stream.print(ui16_Out, DEC);
}

void hexout(Print &stream, uint16_t ui16_Out, uint8_t ui8CountOfDigits)
{
  // 4 digits, 0..FFFF
  if((ui8CountOfDigits > 3) && (ui16_Out < 0x1000))
    stream.print('0');
  if((ui8CountOfDigits > 2) && (ui16_Out < 0x0100))
    stream.print('0');
  if((ui8CountOfDigits > 1) && (ui16_Out < 0x0010))
    stream.print('0');
  stream.print(ui16_Out, HEX);
}

#endif
