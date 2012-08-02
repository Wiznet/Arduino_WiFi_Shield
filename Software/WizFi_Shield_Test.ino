/*
 WizFi210 Arduino test code
 
 Circuit:
 WizFi210 connected to Arduino via SPI
 
 RST: pin 2  // Output
 DRDY: pin 3  // Input
 CSB: pin 4  // output

 MOSI: pin 11  // output
 MISO: pin 12
 SCK: pin 13  // out
 
 Created 15 May 2012
 by Jinbuhm Kim  (jbkim@wiznet, Jinbuhm.Kim@gmail.com)

 */

// WizFi210 communicates using SPI, so include the SPI library:
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SPI.h>

#define DHCP_ENABLE

const byte SPI_ESC_CHAR  = 0xFB;       /* Start transmission indication */
const byte SPI_IDLE_CHAR = 0xF5;       /* synchronous IDLE */
const byte SPI_XOFF_CHAR = 0xFA;       /* Stop transmission indication */
const byte SPI_XON_CHAR = 0xFD;        /* Start transmission indication */      
const byte SPI_INVALID_CHAR_ALL_ONE = 0xFF;    /* Invalid character possibly recieved during reboot */
const byte SPI_INVALID_CHAR_ALL_ZERO = 0x00;    /* Invalid character possibly recieved during reboot */
const byte SPI_LINK_READY = 0xF3;     /* SPI link ready */

#define SSID 		"WIZnetCisco"        // SSID of your AP
#define WPA_PASS 	"0557564860"         // WPA Password

#define Infrastructure	0
#define Ad_hoc		1

#define Client_Mode	0
#define Server_Mode	1

#define Protocol_UDP	0
#define Protocol_TCP	1

// for static IP address
unsigned char IP[4] 		= {192, 168, 88, 123};
unsigned char  Sub[4] 	        = {255, 255, 255, 0};
unsigned char  Gw[4] 		= {192, 168, 88, 1};
unsigned char  SIP[4] 	        = {192, 168, 88, 228};
unsigned int ListenPort = 5000;
unsigned int ServerPort = 5000;


// pins used for the connection with the WizFi210
const int rstPin = 2;
const int dataReadyPin = 3;
const int chipSelectPin = 4;

boolean Wifi_setup = false;

void setup() {
  Serial.begin(9600);
  Serial.println("\r\nSerial Init");
  
  // initalize the  data ready and chip select pins:
  pinMode(dataReadyPin, INPUT);
  pinMode(chipSelectPin, OUTPUT);
  pinMode(rstPin, OUTPUT);
  
  // start the SPI library:
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV128); //
    SPI.begin();

  digitalWrite(chipSelectPin, HIGH);
  digitalWrite(rstPin, HIGH);

  Serial.println("Wi-Fi Reset");
  digitalWrite(rstPin, LOW);
  delay(500);
  
  digitalWrite(rstPin, HIGH);
  delay(6000); // 

  Serial.println("Wi-Fi Init");
  Init();
  Serial.println("Wi-Fi Init Complete");
}

void loop() {
  byte tmp;
  
  if (Wifi_setup == true) {
        
        // if there is data from Wi-Fi side. 
	tmp = WizFI210_Read_Byte();
	if ((tmp != SPI_IDLE_CHAR) &&  (tmp != SPI_INVALID_CHAR_ALL_ZERO) && (tmp != SPI_INVALID_CHAR_ALL_ONE)){
             Serial.print(char(tmp));
	}
        
        // if the connection is established, incoming serial data sent to the peer.
        if (Serial.available() > 0) {
          WizFI210_Write_Byte(Serial.read());
        }

    
  }
}

byte WizFI210_Read_Byte() {
    byte result;
    
    digitalWrite(chipSelectPin, LOW);
    result = SPI.transfer(0);
    digitalWrite(chipSelectPin, HIGH);
    return(result);
}

unsigned int WizFI210_Read_Buf(byte * buf)
{
	int idx = 0;
	byte tmp;
	byte chk = 0;
	
	while(digitalRead(dataReadyPin) == HIGH) {

		tmp = WizFI210_Read_Byte();

		if ((tmp != SPI_IDLE_CHAR) &&  (tmp != SPI_INVALID_CHAR_ALL_ZERO) && (tmp != SPI_INVALID_CHAR_ALL_ONE))	{
			if (tmp == SPI_ESC_CHAR) {
				chk = 1;
			}else {
				if (chk == 1) {
					chk = 0;
					tmp = tmp ^ 0x20;
				}
				buf[idx++] = tmp;
			}
		}
	}

	return(idx);
}


byte SpiByteStuff(byte *thisValue)
 {
 if( (SPI_ESC_CHAR	== *thisValue) ||
	 (SPI_XON_CHAR	== *thisValue) ||
	 (SPI_XOFF_CHAR == *thisValue) ||
	 (SPI_IDLE_CHAR == *thisValue) ||
	 (SPI_INVALID_CHAR_ALL_ONE == *thisValue) ||
	 (SPI_INVALID_CHAR_ALL_ZERO == *thisValue)||
	 (SPI_LINK_READY == *thisValue))
 	{   // Byte stuffing is required
		 *thisValue = *thisValue ^ 0x20;
		 return 0x01;
	 }
	 else
	 {
		 return 0x00;
	 }

 } 

void WizFI210_Write_Byte(byte thisValue) 
{
  if (SpiByteStuff(&thisValue)) {
    digitalWrite(chipSelectPin, LOW);
    // Byte stuffing is required.
    SPI.transfer(SPI_ESC_CHAR);
    digitalWrite(chipSelectPin, HIGH);
    }

    digitalWrite(chipSelectPin, LOW);
    SPI.transfer(thisValue);
    digitalWrite(chipSelectPin, HIGH);
}

void WizFI210_Write_Buf(byte *s)
{
  while (*s != '\0')
  {
    if (SpiByteStuff(s)) {
      digitalWrite(chipSelectPin, LOW);
      // Byte stuffing is required.
      SPI.transfer(SPI_ESC_CHAR);
      digitalWrite(chipSelectPin, HIGH);
    }

    digitalWrite(chipSelectPin, LOW);
    SPI.transfer(*s);
    digitalWrite(chipSelectPin, HIGH);
    s ++;
    }
    delay(10);
}

byte Check_OK(byte * buf, unsigned int TimeOut)
{
	unsigned int idx = 0;
	unsigned int t=0;
	byte tmp;
	char * ret = NULL;
        
	while(1) {

		tmp = WizFI210_Read_Byte();
		if ((tmp != SPI_IDLE_CHAR) &&  (tmp != SPI_INVALID_CHAR_ALL_ZERO) && (tmp != SPI_INVALID_CHAR_ALL_ONE)){
                    
			buf[idx++] = tmp;
                        //if (idx > sizeof(buf)) idx = 0;
                        Serial.print(char(tmp));

			if (tmp == ']') break;
		}

		if (t++ > TimeOut) {
                  //  Serial.println("Time out");
                    break;
                }
		delay(10);
	}

    // flush
    while(digitalRead(dataReadyPin) == HIGH) {
  	WizFI210_Read_Byte();
        delay(10);
    }

	ret = strstr((char *)buf, "[OK]");
	if (ret != NULL) {
          return 1;
        } else {
          return 0;
        }
}

byte IsCommandMode()
{
    byte msg[]="AT\r\n";

    WizFI210_Write_Buf(msg);
    memset(msg, '\0', sizeof(msg));
    return(Check_OK(msg, 2000));
}

void Init(void)
{
   byte Msgbuf[68];
   byte key;

 Serial.println("Send Sync data");
 WizFI210_Write_Byte(SPI_IDLE_CHAR);
 delay(1000);

  while(1) {
	key = WizFI210_Read_Byte();
	if ((key != SPI_IDLE_CHAR) &&  (key != SPI_INVALID_CHAR_ALL_ZERO) && (key != SPI_INVALID_CHAR_ALL_ONE)) {
                // debug
		//Serial.print(char(key));
	}
       if (digitalRead(dataReadyPin) == LOW) break;
        delay(10);
  }
  
 Serial.println("Ok");

        // Send AT
        Serial.print("\r\nSend AT");
	sprintf((char *)Msgbuf, "AT\r");
	WizFI210_Write_Buf(Msgbuf);
	memset(Msgbuf, '\0', sizeof(Msgbuf));
        
	if (Check_OK(Msgbuf, 2000))  Serial.print(" : Success");
	else {
                Serial.println("\r\nSend 2nd AT");
		sprintf((char *)Msgbuf, "AT\r");
		WizFI210_Write_Buf(Msgbuf);
		memset(Msgbuf, '\0', sizeof(Msgbuf));
		if (Check_OK(Msgbuf, 2000))  Serial.println(" : Success");
		else Serial.println(" : Error");
	}
       
      if (Setup_WiFi() == true) {
        Wifi_setup = true;
        Serial.println("\r\nConfiguration OK");    
      }else { 
        Wifi_setup = false;
        Serial.println("\r\nConfiguration fail");    
      }

 
}


boolean Setup_WiFi(void)
{
   byte Msgbuf[128];

    Serial.println("\r\nTest Command");
    
    sprintf((char *)Msgbuf, "AT+WD\r");
    Serial.print((char *)Msgbuf);
    WizFI210_Write_Buf(Msgbuf);
    memset(Msgbuf, '\0', sizeof(Msgbuf));
    if (Check_OK(Msgbuf, 10000)) Serial.println(" : Success");
    else Serial.println(" : Error");
    
	//  AT+WAUTO
	sprintf((char *)Msgbuf, "AT+WAUTO=0,%s\r\n", SSID);
        // Serial.print((char *)Msgbuf);
	WizFI210_Write_Buf(Msgbuf);
	memset(Msgbuf, '\0', sizeof(Msgbuf));
	if (Check_OK(Msgbuf, 1000)) Serial.println(" : Success");
	else Serial.println(" : Error");

	//  AT+WAUTH
	sprintf((char *)Msgbuf, "AT+WAUTH=%d\r\n",0);
        // Serial.print((char *)Msgbuf);
	WizFI210_Write_Buf(Msgbuf);
	memset(Msgbuf, '\0', sizeof(Msgbuf));
	if (Check_OK(Msgbuf, 1000)) Serial.println(" : Success");
	else Serial.println(" : Error");

	//  AT+WWPA
	sprintf((char *)Msgbuf, "AT+WWPA=%s\r\n",WPA_PASS);
        // Serial.print((char *)Msgbuf);
	WizFI210_Write_Buf(Msgbuf);
	memset(Msgbuf, '\0', sizeof(Msgbuf));
	if (Check_OK(Msgbuf, 1000)) Serial.println(" : Success");
	else Serial.println(" : Error");

#ifdef DHCP_ENABLE
	//  AT+NDHCP
	sprintf((char *)Msgbuf, "AT+NDHCP=%d\r\n",1);
        // Serial.print((char *)Msgbuf);
	WizFI210_Write_Buf(Msgbuf);
	memset(Msgbuf, '\0', sizeof(Msgbuf));
	if (Check_OK(Msgbuf, 1000)) Serial.println(" : Success");
	else Serial.println(" : Error");
#else
	//  AT+NDHCP
	sprintf((char *)Msgbuf, "AT+NDHCP=%d\r\n",0);
        // Serial.print((char *)Msgbuf);
	WizFI210_Write_Buf(Msgbuf);
	memset(Msgbuf, '\0', sizeof(Msgbuf));
	if (Check_OK(Msgbuf, 1000)) Serial.println(" : Success");
	else Serial.println(" : Error");

	//  AT+NSET
	sprintf((char *)Msgbuf, "AT+NSET=%d.%d.%d.%d,%d.%d.%d.%d,%d.%d.%d.%d\r\n", 
					IP[0],IP[1],IP[2],IP[3],Sub[0],Sub[1],Sub[2],Sub[3],Gw[0],Gw[1],Gw[2],Gw[3]);
        // Serial.print((char *)Msgbuf);
	WizFI210_Write_Buf(Msgbuf);
	memset(Msgbuf, '\0', sizeof(Msgbuf));
	if (Check_OK(Msgbuf, 1000)) Serial.println(" : Success");
	else Serial.println(" : Error");
#endif

	//  AT+NAUTO
	sprintf((char *)Msgbuf, "AT+NAUTO=%d,%d,,%d\r\n",Server_Mode, Protocol_TCP, ListenPort);
        // Serial.print((char *)Msgbuf);
	WizFI210_Write_Buf(Msgbuf);
	memset(Msgbuf, '\0', sizeof(Msgbuf));
	if (Check_OK(Msgbuf, 1000)) Serial.println(" : Success");
	else Serial.println(" : Error");

	//  AT+XDUM
	sprintf((char *)Msgbuf, "AT+XDUM=%d\r\n", 0); //  1: Uart message is disable, 0: enable
        // Serial.print((char *)Msgbuf);
	WizFI210_Write_Buf(Msgbuf);
	memset(Msgbuf, '\0', sizeof(Msgbuf));
	if (Check_OK(Msgbuf, 1000)) Serial.println(" : Success");
	else Serial.println(" : Error");

/*
        // MAC address writing
	sprintf((char *)Msgbuf, "AT+NMAC=00:08:DC:18:76:57\r\n"); 
        Serial.print((char *)Msgbuf);
	WizFI210_Write_Buf(Msgbuf);
	memset(Msgbuf, '\0', sizeof(Msgbuf));
	if (Check_OK(Msgbuf, 1000)) Serial.println(" : Success");
	else Serial.println(" : Error");
*/	
	//  ATA
	sprintf((char *)Msgbuf, "ATA\r\n");
        //Serial.print((char *)Msgbuf);
	WizFI210_Write_Buf(Msgbuf);
	memset(Msgbuf, '\0', sizeof(Msgbuf));
	if (Check_OK(Msgbuf, 20000)) {
          Serial.println(" : Success");
          return true;
        }
	else {
          Serial.println(" : Error");
          return false;
        }

}

