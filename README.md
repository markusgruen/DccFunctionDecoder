# DccFunctionDecoder

Hier klicken für deutsch: [Deutsche Doku](https://github.com/markusgruen/DccFunctionDecoder_Dev/blob/master/README_DE.md)

---

A simple DCC function decoder with 4 AUX channels. Compiles for an ATtiny402

Functionality:
- CV configurable
- 4 individual outputs
- 3 modes available for each output: FADE, BLINK, NEON
- Function mapping for F0...F20 for each output
- short and long address
- consist address

## Hardware
KiCad files are available in the `Hardware`-folder

The PCB has a size of approx. 7 by 7 mm. PCB is double-sided and only has standard components in the size of 0603 or larger to enable hand soldering of the decoder.

<img src="/Doc/DccFunctionDecoder_4ch_PinOut.png" width="600px">

## Software
The software can be compiled and flashed using the Arduino environment.

## Usage
### Flashing the software
You will require a standard USB-UART adapter (5V compatible). Connect the adapter as shown with the decoder. You **MUST NOT** connect the decoder's 5V with the UART-adapter. The LDO on the decoder does not like being reverse powered. Switch on the rail's power supply.

<img src="/Doc/DccFunctionDecoder_4ch_Programming.png" width="800px">

Select the appropriate COM port in the Arduino IDE and select `Sketch -> Upload using Programmer` (or hit **CTRL+SHIFT+U**). Just hitting the `Upload`-Button (or pressing **CTRL+U**) will not work.

### Using the decoder in a locomotive or wagon
#### Connecting LEDs
Connect the LEDs as shown. Use appropriate resistors. The maximum current should be limited to 2 mA per LED. That does sound little but usually it is enough. Otherwise the LDO on the decoder gets too hot. 

<img src="/Doc/DccFunctionDecoder_4ch_Wiring.png" width="800px">

#### Programming CVs
The decoder ONLY supports Programming on Main (POM) mode. It cannot generate an ACK. For CVs the following table below. The decoder will confirm each CV write with a short flash of all AUX channels. 

##### Programming the address
Special note on programming the address: If you want to change the decoder's address, you must do so in the following order:
1. program CV3 or CV17 and CV18 as desired.
2. program CV29 = 0 (short address) or CV29 = 32 (long address)

From then on, the decoder will only accept CV writes or commands to the new address. (it will of course still accept commands to its consist address)

##### Decoder reset
The decoder can be reset to its default CV values by writing CV8 = 8. This is confirmed by multible short flashes of all AUX channels.

##### CV table

| CV | Type | range | default Value | Description |
|----------|----------|----------|----------|----------|
| 1 | address | 1...127 | 3 | short address |
| 17 | long Address high byte | 192...255 | 0 | CV17 = 192 + (address / 256) |
| 18 | long address low byte  | 0...255   | 0 | CV18 = address mod 256 |
| 19 | consist address  | 0...255   | 0 | consist address |
| 29 | configuration byte  | 0...255   | 0 | CV29 = 0: short address;<br>CV29 = 32: long address |
| 33...35<br>36...38<br>39...41<br>42...44 | FunctionMap AUX n <br> (3 bytes each)  | 0...255  | CV33 = 0b00000010 (F1) <br> CV36 = 0b00000100 (F2) <br> CV39 = 0b00001000 (F3) <br> CV42 = 0b00010000 (F4) <br> all others = 0  | Bit 0 = 1: AUX n controlled by F0<br> Bit 1 = 1: AUX1 controlled by F1 <br>....<br> Bit 20 = 1 : AUX n controlled by F20 <br> Bit 21: reserved <br> Bit 21 = 1: AUX n only active when forward <br> Bit 22 = 1: AUX n only active when reverse |
| 57 <br> 58 <br> 59 <br> 60  | Output mode AUX n  | 0...2   |   0 | 0 = fade <br> 1 = neon <br> 2 = Blink |
| 65 <br> 66 <br> 67 <br> 68  | dimm value AUX n   | 0...255 | 255 | 0 = off <br> ... <br> 255 = full brightness |
| 73 <br> 74 <br> 75 <br> 76  | fade speed AUX n   | 0...255 |   0 | 0 = instant on/off <br> ... <br> 255 = very slow fading |
| 97 <br> 98 <br> 99 <br> 100 | blink on time AUX n  | 0...255  |   80 | value * 10ms = on-time during blink <br> (only active when CV57...60 = 2) |
| 105 <br> 106 <br> 107 <br> 108 | blink off time AUX n  | 0...255  |   150 | value * 10ms = off-time during blink <br> (only active when CV57...60 = 2) |


   
## License and Legal
Copyright 2025 by Markus Grün

Software licensed under GNU General Public License v3.0, see license file in `Software`-folder

Hardware (Schematics, Layout, etc.) licensed under CERN-OHL-P v2 or later, see license file in `Hardware`-folder

THE SOFTWARE AND SOURCES ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR SOURCES OR THE USE OR OTHER DEALINGS IN THE SOFTWARE OR SOURCES.






