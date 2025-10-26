# DccFunctionDecoder

Ein einfacher DIY DCC-Funktionsdecoder mit 4 AUX-Kanälen, basierend auf einem ATtiny402.

Funktionen:
- konfigurierbare CVs
- nutzbare Funktionen F0...F20
- 4 individuelle Ausgänge  
- 3 Modi pro Ausgang verfügbar: FADE, BLINK, NEON  
- kurze und lange Adresse  
- Consist-Adresse  

## Hardware
KiCad-Dateien sind im Ordner `Hardware` abgelegt.  

Die Leiterplatte hat eine Größe von ca. 7 x 7 mm. Sie ist doppelseitig und enthält nur Standardbauteile in 0603 oder größer, so dass sie auch von Hand gelötet werden kann.

<img src="/Doc/DccFunctionDecoder_4ch_PinOut.png" width="600px">

## Software
Die Software kann mit der normalen Arduino-Umgebung kompiliert und geflasht werden.  

## Verwendung
### Flashen der Software
Ein Standard-USB-UART-Adapter (5V-kompatibel) wird benötigt. Der Decoder wird wie unten dargestellt mit einer Diode und einem Widerstand mit dem UART-Adapter verbunden. **WICHTIG:** Die 5V des Decoders dürfen **NICHT** mit dem UART-Adapter verbunden werden. Der LDO auf dem Decoder verträgt keine Rückspeisung. Die Gleisspannung muss angeschaltet sein, damit der Decoder mit Spannung versorgt wird.  

<img src="/Doc/DccFunctionDecoder_4ch_Programming.png" width="800px">

Zum Flashen der Software in der Arduino-IDE den passenden COM-Port auswählen und dann über `Sketch -> Upload using Programmer` (oder **STRG+SHIFT+U**) die Software auf den Decoder flashen. Der normale `Upload`-Knopf (oder **STRG+U**) funktioniert hier **nicht**.  

### Erstinbetriebnahme nach dem Flashen der Software
Direkt nach dem Flashen der Software ist der Decoder noch nicht einsatbereit, da das EEPROM des Prozessors noch nicht beschrieben ist. Die Dekoderadresse ist damit unbekannt. Um den EEPROM mit den Default-Werten (u.a. Adresse = 3) zu beschreiben, muss er wie im folgenden Abschnitt beschrieben angeschlossen werden. Dann kann ein Decoder-Reset durchgeführt werden, in dem man die "Magic Address = 10239" nutzt und CV8 = 8 beschreibt. Unter dieser Adresse lässt sich der Dekoder immer beschreiben. Das ist zum Beispiel nützlich, falls man mal die Adresse des Dekoders vergessen hat. Das Beschreiben der CVs im POM Modus funktioniert bei DecoderPro wie folgt:
<img src="/Doc/DccFunctionDecoder_4ch_FactoryReset.png" width="800px">

### Einsatz in einer Lok oder einem Waggon
Die LEDs werden wie unten gezeigt mit geeigneten Vorwiderständen (max. 2mA pro Kanal!) direkt an den Beinchen des Mikrocontrollers angeschlossen. Ein Strom von 2mA klingt erstmal nach wenig, reicht aber in der Regel aus. Andernfalls wird der LDO auf dem Decoder zu heiß.  

<img src="/Doc/DccFunctionDecoder_4ch_Wiring.png" width="800px">

### CVs programmieren
Der Decoder unterstützt **ausschließlich Programming on Main (POM)**. Er kann kein ACK-Signal erzeugen. Die CVs sind in der untenstehenden Tabelle näher beschrieben. Der Decoder bestätigt jedes erfolgreiches Schreiben eines CV-Wertes mit einem kurzen Aufblitzen aller AUX-Kanäle.

#### Programmieren der Adresse
**Wichtiger Hinweis zur Adress-Programmierung:**  
Um die Adresse des Decoders zu ändern ist folgendes Vorgehen notwendig:
1. CV3 oder CV17 und CV18 nach Wunsch programmieren.  
2. CV29 = 0 (kurze Adresse) oder CV29 = 32 (lange Adresse) programmieren.  

Ab diesem Moment akzeptiert der Decoder nur noch CV-Schreibbefehle oder Kommandos an die **neue Adresse**. (Befehle an die Consist-Adresse wird natürlich weiterhin akzeptiert.)  

#### Decoder reset
Der Decoder kann durch Schreiben von CV8 = 8 auf Werkseinstellungen zurückgesetzt werden. Dies wird durch mehrmaliges Aufblitzen aller AUX-Kanäle angezeigt.

#### CV-Tabelle

| CV | Typ | Bereich | Standardwert | Beschreibung |
|----------|----------|----------|----------|----------|
| 1 | Adresse | 1...127 | 3 | kurze Adresse |
| 17 | lange Adresse High-Byte | 192...255 | 195 | CV17 = 192 + (Adresse / 256) |
| 18 | lange Adresse Low-Byte  | 0...255   | 232 | CV18 = Adresse modulo 256 |
| 19 | Consist-Adresse  | 0...255   | 0 | Consist-Adresse |
| 29 | Konfigurationsbyte  | 0...255   | 0 | CV29 = 0: kurze Adresse;<br>CV29 = 32: lange Adresse |
| 33...35<br>36...38<br>39...41<br>42...44 | Funktionszuordnung AUX n <br> (jeweils 3 Bytes)  | 0...255  | CV33 = 0b00000010 (F1)<br> CV36 = 0b00000100 (F2) <br> CV39 = 0b00001000 (F3) <br> CV42 = 0b00010000 (F4) <br> alle anderen = 0  | Bit 0 = 1: AUX n durch F0 gesteuert<br> Bit 1 = 1: AUX n durch F1 gesteuert <br>....<br> Bit 20 = 1 : AUX n durch F20 gesteuert <br> Bit 21: reserviert <br> Bit 22 = 1: AUX n nur aktiv bei Vorwärtsfahrt <br> Bit 23 = 1: AUX n nur aktiv bei Rückwärtsfahrt |
| 57 <br> 58 <br> 59 <br> 60  | Ausgangsmodus AUX n  | 0...2   |   0 | 0 = Fade <br> 1 = Neon <br> 2 = Blink |
| 65 <br> 66 <br> 67 <br> 68  | Dimmwert AUX n   | 0...255 | 255 | 0 = aus <br> ... <br> 255 = volle Helligkeit |
| 73 <br> 74 <br> 75 <br> 76  | Fade-Geschwindigkeit AUX n   | 0...255 |   0 | 0 = hart an/aus <br> ... <br> 255 = sehr langsames Fading |
| 97 <br> 98 <br> 99 <br> 100 | Blink-Ein-Zeit AUX n  | 0...255  |   80 | Wert * 10ms = Ein-Zeit beim Blinken <br> (nur aktiv, wenn CV57...60 = 2) |
| 105 <br> 106 <br> 107 <br> 108 | Blink-Aus-Zeit AUX n  | 0...255  |   150 | Wert * 10ms = Aus-Zeit beim Blinken <br> (nur aktiv, wenn CV57...60 = 2) |

## Lizenz und Rechtliches
Copyright 2025 von Markus Grün  

Software lizenziert unter der **GNU General Public License v3.0**, siehe Lizenzdatei im Ordner `Software`.  

Hardware (Schaltpläne, Layouts usw.) lizenziert unter der **CERN-OHL-P v2 oder später**, siehe Lizenzdatei im Ordner `Hardware`.  

THE SOFTWARE AND SOURCES ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR SOURCES OR THE USE OR OTHER DEALINGS IN THE SOFTWARE OR SOURCES.
