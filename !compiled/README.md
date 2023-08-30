"Ready to go files"  for people who got problems with compilators, IDF etc(I know how annoying this could be), under Windows.
Just download prepared files for arduino and esp32 from Blueretro and OGX360 folders(Use "Download raw file" option).
## ESP32  part :
Dowload [flash download tool](https://www.espressif.com/sites/default/files/tools/flash_download_tool_3.9.5.zip) 
Run program and set :
Chip Type : ESP32
WorkMode : Develop

<img src="./Images/flash1.jpg" alt="flash1"/>  



Put all downloaded bin files into flash tool (3 dots), and set everything like on this screen.

<img src="./Images/flash2.jpg" alt="flash2"/>  

Change COM to Yours -  Determine what COM number Esp32 appears in the device manager.
Click START - and done.
## Arduino part :

* Download avrdude-6.3-mingw32.zip for Windows from  [avrdude](http://download.savannah.gnu.org/releases/avrdude/), and unzip it where you wish to.
*   Determine what COM number Arduino appears in the device manager.
* Go into command prompt in Windows, change folder to avrdudes folder
* write command:
avrdude -C avrdude.conf -F -p atmega32u4 -c avr109 -b 57600 -P COMx -Uflash:w:ogx360.hex:i
* !ALter COMx with Yours for arduino!
* Done.
## Connections
Wiring
For Player 1: Connect pins 6,7 on the Arduino Player 1  to Ground
For Player 2: Connect pin 7 on the Arduino Player 2  to Ground
For Player 3: Connect pin 6 on the Arduino Player 3  to Ground
For Player 4: Don't connect any additional pins to ground

<img src="./Images/Arduino pinouts.jpg" alt="flash2"/>  


Connect the 5v - RAW(pin 1) on  all Arduino together with 5V pin on ESP32.
Connect  ground of all the boards.
Connect pin 2 and 3 on all Arduinos together.
Connect pin 2 of that set to pin 22 on the esp32 and pin 3 to pin 21 on the esp32.

<img src="./Images/Esp32 pinouts.jpg" alt="flash2"/>  

BE AVARE: esp32 logic is 3,3Volt!
Arduino pro micro (leonardo) needed for ogx360 is 16MHz version, 5Volt - that means his logic is also 5Volt!! Connecting those two, CAN DAMAGE YOUR ESP32!!!!
However works, question is: for how much long??
I have used for safety Bidirectional Level shifter
like this one :

<img src="./Images/Level Shifter.jpg" alt="flash2"/>  

Just search for example "4 Channels IIC I2C Logic Level Converter Bi-Directional Module 3.3V to 5V Shifter for Arduino" in Your favourite shopping site.

The choice is Yours. 





