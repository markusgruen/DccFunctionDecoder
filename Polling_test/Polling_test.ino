// volatile uint64_t vBitstream;
// volatile uint8_t vNumReceivedBits;
volatile uint16_t vTimerCompareValue;

#define RINGBUF_SIZE 8
#define RINGBUF_MASK (RINGBUF_SIZE - 1)
#define FILTER_LENGTH 5

constexpr uint16_t FILTER_SHIFT_MASK = (1<<FILTER_LENGTH);
constexpr uint8_t FILTER_THRESHOLD = (FILTER_LENGTH+1)/2;


typedef struct {
  uint8_t buffer[RINGBUF_SIZE];     // NICHT volatile
  volatile uint8_t head;            // wird von ISR geschrieben
  volatile uint8_t tail;            // wird vom Hauptprogramm gelesen
} RingBuffer;

RingBuffer ringbuf;


#define THRESHOLD 100
#define DCC_SIGNAL_SAMPLE_TIME 200


void setup() {
  pinMode(PIN_PB1, OUTPUT);
  pinMode(PIN_PB0, OUTPUT);
  PORTB.OUTCLR = PIN1_bm;  // vor Beginn
  delay(1);

  // Disable TCB before configuring
  TCB0.CTRLA = 0;

  // Use CLK_PER (20 MHz), enable, no prescaler
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc;  // Clock source: no division

  // Enable CMP interrupt, use Timebase (not periodic)
  TCB0.CTRLB = TCB_CNTMODE_INT_gc;     // Timebase mode (count up, interrupt on compare)
  //TCB0.CCMP = vTimerCompareValue;  // First compare match after 200 ticks
  //TCB0.INTCTRL = TCB_CAPT_bm;          // Enable compare match interrupt

  TCB0.CTRLA |= TCB_ENABLE_bm;      // Enable timer

}

void loop() {
  bool a;
  //uint64_t testValues = 0b1000000000000000000001001101111111111110110010000000000000101111; 
  uint64_t testValues = 0b1000000111111000000111111000000111111000000111111000000111111000; 

  for(int ii=0; ii<8; ii++){
    uint64_t testValues_here = testValues;
    for(int i=0; i<64; i++) {

      a = testValues_here & 0b1;
      //PORTB.OUTSET = PIN1_bm;  // vor Beginn
      testFunction(a);
      //PORTB.OUTCLR = PIN1_bm;  // vor Beginn
      testValues_here >>= 1;
      delayMicroseconds(10);
      // for(long i=0; i<1000; i++) {
      //   __asm__ __volatile__("nop");
      //   __asm__ __volatile__("nop");
      //   __asm__ __volatile__("nop");
      // }
    }
  }
  while(1);
}


void testFunction(bool isHigh) {
  PORTB.OUTSET = PIN1_bm;  // vor Beginn
// IDEE:
// vBitstream als uint8_t implementieren (viel weniger Rechenaufwand)
// wenn vNumReceivedBits == 8 ist, einen weiteren Interrupt generieren, durch manuelles Setzen des Interruptflags
// in der zweiten ISR vBitstream wegspeichern und vNumReceivedBits zu Null setzen. ggf. kann vNumReceivedBits auch lokal und static sein
  static uint8_t sFilterShiftRegister = 0;
  static uint8_t sFilterValue = 0;
  static bool sOldFilterResult = 0;

  static uint16_t sOldTimerValue = 0;
  uint16_t currentTimerValue = TCB0.CNT;
  static uint8_t sLastHalfBit = 0xFF;
  
  static uint8_t sBitstreamByte = 0;
  static uint8_t sBitCount = 0;
  
  // Pegel messen
  //bool isHigh = PORTA.IN & DccSignalReceiver::mDccPinMask;
  __asm__ __volatile__("nop");
  __asm__ __volatile__("nop");
  __asm__ __volatile__("nop");

// moving average of the last 5 measurements
  sFilterShiftRegister <<= 1;
  sFilterShiftRegister |= (isHigh & 0b1);
  sFilterValue += isHigh;
  sFilterValue -= (bool)(sFilterShiftRegister & FILTER_SHIFT_MASK);

  // edge detection
  bool thisFilterResult = sFilterValue >= FILTER_THRESHOLD;
  bool edgeDetected = thisFilterResult != sOldFilterResult;
  sOldFilterResult = thisFilterResult;
  
  // evaluate received bit
  if(edgeDetected) {
    uint8_t thisHalfBit = (currentTimerValue - sOldTimerValue) < THRESHOLD;
    sOldTimerValue = currentTimerValue;

    if (sLastHalfBit == 0xFF) {
      // if this is the first half-bit
      sLastHalfBit = thisHalfBit;
    }
    else {
      // if this is the second half-bit
      if (thisHalfBit == sLastHalfBit) {
        sBitstreamByte |= (thisHalfBit << sBitCount++);

        // save byte to ringbuffer if 8 bits have been received
        if(sBitCount == 8) {
          uint8_t next = (ringbuf.head + 1) & RINGBUF_MASK;
          ringbuf.buffer[ringbuf.head] = sBitstreamByte;
          ringbuf.head = next;
          sBitCount = 0;
          PORTB.OUTTGL = PIN0_bm;  // Debug-Toggle
        }
      }

      sLastHalfBit = 0xFF; // immer zurücksetzen nach einem Paar
    }
  }

  // prepare timer for next interrupt
  vTimerCompareValue += DCC_SIGNAL_SAMPLE_TIME;
  TCB0.CCMP = vTimerCompareValue;
  __asm__ __volatile__("nop");
  //TCB0.INTFLAGS = TCB_CAPT_bm;

  PORTB.OUTCLR = PIN1_bm;  // vor Beginn
}
