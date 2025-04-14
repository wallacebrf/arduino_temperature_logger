//********************************************************************
//add needed include files
//********************************************************************
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <Ethernet.h>

#include <OneWire.h>
#include <avr/wdt.h>
#include <Dns.h>

//********************************************************************
//Define needed variables
//********************************************************************

//The following lines are for different loggers, uncomment the needed one and comment out the un-needed ones

//********************************************************************
//server logger variables 
//********************************************************************
//byte mac[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX };
//#define left_sensor_pin 38
//#define middle_sensor_pin 44
//#define right_sensor_pin 2
//#define health_id "xxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
//byte sesnor_name = 1;
// Set the static IP address to use if the DHCP fails to assign
//byte ip[] = { 192, 168, 1, 45 }; 
//byte myDns[] = { 192, 168, 1, 1 }; 
//byte gateway[] = { 192, 168, 1, 1 }; 
//byte subnet[] = { 255, 255, 255, 0 }; 

//********************************************************************
//PIT logger variables 
//********************************************************************
//byte mac[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX };
//#define left_sensor_pin 7
//#define middle_sensor_pin 6
//#define right_sensor_pin 2
//#define health_id "xxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
//byte sesnor_name = 2;
// Set the static IP address to use if the DHCP fails to assign
//byte ip[] = { 192, 168, 1, 44 }; 
//byte myDns[] = { 192, 168, 1, 1 }; 
//byte gateway[] = { 192, 168, 1, 1 }; 
//byte subnet[] = { 255, 255, 255, 0 }; 

//********************************************************************
//equipment cabinet logger variables 
//********************************************************************
//byte mac[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX };
//#define left_sensor_pin 22
//#define middle_sensor_pin 23
//#define right_sensor_pin 2
//#define health_id "xxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
//byte sesnor_name = 3;
// Set the static IP address to use if the DHCP fails to assign
//byte ip[] = { 192, 168, 4, 3 }; 
//byte myDns[] = { 192, 168, 4, 1 }; 
//byte gateway[] = { 192, 168, 4, 1 }; 
//byte subnet[] = { 255, 255, 255, 0 }; 

//********************************************************************
//garage temp logger and AC unit interlock variables 
//********************************************************************
//byte mac[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX };
#define left_sensor_pin A0
#define middle_sensor_pin A1
#define right_sensor_pin A2
//#define health_id "xxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
byte sesnor_name = 4;
// Set the static IP address to use if the DHCP fails to assign
byte ip[] = { 192, 168, 1, 19 }; 
byte myDns[] = { 192, 168, 1, 1 }; 
byte gateway[] = { 192, 168, 1, 1 }; 
byte subnet[] = { 255, 255, 255, 0 }; 


//********************************************************************
//General Variables
//********************************************************************
IPAddress healthcheckio_IP;

//Variables for the logging server and the AC interlock arduino
byte serverip[] = {192, 168, 1, 13};			//target server IP address running the PHP script to ingest temperature data
byte ac_controller[] = {192, 168, 1, 254};		//target arduino controlling AC on/off relay


byte skip_network=0;
float average_temp;
long interval_temp = 60000;						// READING INTERVAL --> how often is a data sample taken, this default value = every 60 seconds
long interval_heartbeat = 900000;				// READING INTERVAL --> how often is healthchecks.io contacted for a heartbeat update? value = 15 minutes 
long interval_dns_refresh = 86400000;			// READING INTERVAL --> how often is healthchecks.io contacted for a heartbeat update? value = 24 hours
byte debug=0;
const char* ip_to_str(const uint8_t*);
EthernetClient client;
DNSClient dnClient;
long previousMillis_temp = 0;
long previousMillis_dns_refresh = 0;
long previousMillis_heartbeat = 0;
unsigned long currentMillis = 0;
float middle_sensor;                            
float left_sensor; 
byte temp_scale = 1;
float right_sensor;  

//********************************************************************
//function to nicely format an IP address.
//********************************************************************
const char* ip_to_str(const uint8_t* ipAddr)
{
	static char buf[16];
	sprintf(buf, "%d.%d.%d.%d\0", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
	return buf;
}

//********************************************************************
//Setup function 
//********************************************************************
void setup() { 
  wdt_enable(WDTO_8S);
	Serial.begin(115200);
	if (sesnor_name == 1) {
		Serial.println(F("Server Room Data Logger....."));
	}else if (sesnor_name == 2) {
		Serial.println(F("Utility Pit Data Logger....."));
	}else if (sesnor_name == 3) {
		Serial.println(F("Equipment Cabinet Data Logger....."));
	}else if (sesnor_name == 4) {
		Serial.println(F("Garage (Outside Temp) Data Logger....."));
	}
 
//********************************************************************
// start the Ethernet connection:
//********************************************************************
	Serial.println(F("Initializing Ethernet with DHCP:"));
	if (Ethernet.begin(mac) == 0) {
		Serial.println(F("Failed to Configure Ethernet Using DHCP"));
		// Check for Ethernet hardware present
		if (Ethernet.hardwareStatus() == EthernetNoHardware) {
			Serial.println(F("Ethernet Shield Was Not Found.  Sorry, Can't Run Without Hardware."));
			while (true) {
				delay(1); // do nothing, no point running without Ethernet hardware. due to watchdog, this will cause the ardiuno to reset over and over trying to find the sheild 
			}
		}
		if (Ethernet.linkStatus() == LinkOFF) {
			Serial.println(F("Ethernet cable is not connected."));
		}
		// try to configure using IP address instead of DHCP:
		Ethernet.begin(mac, ip, myDns, gateway, subnet);
		skip_network=1;
		// give the Ethernet shield a second to initialize:
		delay(1000);
		Serial.print(F("Static assigned IP "));
		Serial.println(Ethernet.localIP());
	} else {
		// give the Ethernet shield a second to initialize:
		delay(1000);
		Serial.print(F("DHCP assigned IP "));
		Serial.println(Ethernet.localIP());
	}
	if (Ethernet.hardwareStatus() == EthernetW5100) {
		Serial.println(F("W5100 Ethernet controller detected."));
	}else if (Ethernet.hardwareStatus() == EthernetW5200) {
		Serial.println(F("W5200 Ethernet controller detected."));
	}else if (Ethernet.hardwareStatus() == EthernetW5500) {
		Serial.println(F("W5500 Ethernet controller detected."));
	}
  wdt_reset();
  
//********************************************************************
// start DNS subsystem
//********************************************************************  
	dnClient.begin(Ethernet.dnsServerIP());
	if(dnClient.getHostByName("hc-ping.com",healthcheckio_IP) == 1) {
		Serial.print(F("hc-ping.com = "));
		Serial.println(healthcheckio_IP);
		Serial.println("");
		Serial.println("");
	}else{ 
		Serial.println(F("DNS Lookup Failed"));
	}

  if (client.connect(healthcheckio_IP,80)) {
			Serial.println(F("Heartbeat Client Connected"));
			Serial.println("");
			client.print(F("GET /"));
			client.print(health_id);
			client.println(F(" HTTP/1.1"));
			client.println(F("Host: hc-ping.com"));
			client.println(F("Connection: close"));
			client.println();
			client.stop();
		} else{
			Serial.print(F("Heartbeat could not connect to server"));
		}
    wdt_reset();
}

//********************************************************************
// Primary LOOP
//********************************************************************
void loop(){

//********************************************************************
//maintain DHCP IP lease
//********************************************************************
	if(skip_network==0){
		switch (Ethernet.maintain()) {
		case 1:
			//renewed fail
			Serial.println(F("Error: DHCP renew Fail"));
			break;
		case 2:
			//renewed success
			Serial.println(F("DHCP Renewed Success"));
			//print your local IP address:
			Serial.print(F("My IP address: "));
			Serial.println(Ethernet.localIP());
			break;
		case 3:
			//rebind fail
			Serial.println(F("Error: DHCP Rebind Fail"));
			break;
		case 4:
			//rebind success
			Serial.println(F("DHCP Rebind Success"));
			//print your local IP address:
			Serial.print(F("My IP address: "));
			Serial.println(Ethernet.localIP());
			break;
		default:
			//nothing happened
			break;
		}
	}
    wdt_reset();

//********************************************************************
//Get temperature sensor data
//********************************************************************
  currentMillis = millis();
  middle_sensor =getTemp(middle_sensor_pin);
  left_sensor =getTemp(left_sensor_pin);
	if (sesnor_name == 4) {
    right_sensor =getTemp(right_sensor_pin);
	}

//********************************************************************
//Print out current temperatures and system status to serial port for status
//********************************************************************
    Serial.println(F("----------------------------------------------------------------"));
	Serial.println(F("Version 3.0 3/4/2025"));
	if (sesnor_name == 1) {
		Serial.println(F("Server Room Data Logger"));
	}else if (sesnor_name == 2) {
		Serial.println(F("Utility Pit Data Logger"));
	}else if (sesnor_name == 3) {
		Serial.println(F("Equipment Data Logger"));
	}else if (sesnor_name == 4) {
		Serial.println(F("Garage (Outside Temperature + AC Interlock Source) Data Logger"));
	}
    Serial.println(F("TEMPERATURE DATA"));
    Serial.println(F("------------------------"));
    Serial.print(F("middle_temp: "));
    Serial.println(middle_sensor);  
    Serial.print(F("cold_side_temp: "));
    Serial.println(left_sensor); 
	if (sesnor_name == 4) {
		Serial.print(F("high_side_temp: "));
		Serial.println(right_sensor); 
	}
    Serial.println(); 
    Serial.println(F("NETWORK STATUS"));
    Serial.println(F("------------------------"));
    if (Ethernet.hardwareStatus() == EthernetW5100) {
		Serial.println(F("W5100 Ethernet Controller Detected."));
    }else if (Ethernet.hardwareStatus() == EthernetW5200) {
		Serial.println(F("W5200 Ethernet Controller Detected."));
    }else if (Ethernet.hardwareStatus() == EthernetW5500) {
		Serial.println(F("W5500 Ethernet Controller Detected."));
    }
    Serial.print(F("hc-ping.com: "));
    Serial.println(healthcheckio_IP);
    Serial.print(F("Assigned IP: "));
    Serial.println(Ethernet.localIP());
    Serial.print(F("Assigned Network Gateway: "));
    Serial.println(Ethernet.gatewayIP());
    Serial.print(F("Assigned DNS Server: "));
    Serial.println(Ethernet.dnsServerIP());
    Serial.print(F("Assigned Subnet Mask: "));
    Serial.println(Ethernet.subnetMask());

//********************************************************************
//send current temperature data to AC controller interlock
//********************************************************************
	if (sesnor_name == 4) {
		Serial.println(F("AC INTERLOCK CONTROLLER DATA TRANSMISSION"));
		Serial.println(F("------------------------"));

    //********************************************************************
    //validate data to ensure if one or more sensors are offline or reporting bad data (CRC fails etc), then ignore the bad sensor data but try keeping all good data
    //********************************************************************

    if (middle_sensor == 4096.0 && left_sensor <= 120.0 && right_sensor <= 120.0){
      //middle sensor is reporting bad data, but other sensors are reporting good data, keep the good data
      average_temp=(left_sensor + right_sensor)/2.0;
    }else if (left_sensor == 4096.0 && middle_sensor <= 120.0 && right_sensor <= 120.0){
       //left sensor is reporting bad data, but other sensors are reporting good data, keep the good data
      average_temp=(middle_sensor + right_sensor)/2.0;
    }else if (right_sensor == 4096.0 && middle_sensor <= 120.0 && left_sensor <= 120.0){
       //right sensor is reporting bad data, but other sensors are reporting good data, keep the good data
      average_temp=(middle_sensor + left_sensor)/2.0;
     }else if (left_sensor == 4096.0 && middle_sensor == 4096.0 && right_sensor <= 120.0){
       //left and middle sensor is reporting bad data, but the third sensor is reporting good data, keep the good data
      average_temp=right_sensor;
    }else if (middle_sensor == 4096.0 && right_sensor == 4096.0 && left_sensor <= 120.0){
       //right and middle sensor is reporting bad data, but the third sensor is reporting good data, keep the good data
      average_temp=left_sensor;
    }else if (left_sensor == 4096.0 && right_sensor == 4096.0 && middle_sensor <= 120.0){
       //left and right sensor is reporting bad data, but the third sensor is reporting good data, keep the good data
      average_temp=middle_sensor;
    }else if (left_sensor <= 120.0 && right_sensor <= 120.0 && middle_sensor <= 120.0){
       //all sensors report correctly
       average_temp=(middle_sensor + left_sensor + right_sensor)/3.0;
    }else if (left_sensor == 4096.0 && right_sensor == 4096.0 && middle_sensor == 4096.0){
      //make temp above 120 degrees as this will cause the AC interlock to ignore the data
      average_temp=4096.0;
      delay(5000);
    }


		if (client.connect(ac_controller,80)) { 
			Serial.println(F("AC Controller Client Connected"));
			Serial.print(F("Average Temperature: "));
			Serial.println(average_temp);
			client.print(average_temp);
			client.stop();
			client.stop();
		} else{
			Serial.println(F("Could not connect to AC Interlock Controller"));
		}
	}
    Serial.println(F("----------------------------------------------------------------"));
    Serial.println(); 
    Serial.println(); 

//********************************************************************
//Refresh DNS lookup of health checks server
//********************************************************************
	if(currentMillis - previousMillis_dns_refresh > interval_dns_refresh) { // READ ONLY ONCE PER INTERVA
	  previousMillis_dns_refresh = currentMillis; 
	  if(dnClient.getHostByName("hc-ping.com",healthcheckio_IP) == 1) {
		  Serial.print(F("Daily DNS lookup Refresh: hc-ping.com = "));
		  Serial.println(healthcheckio_IP);
		  Serial.println("");
		  Serial.println("");
	  }else{ 
		  Serial.println(F("DNS Lookup Failed"));
	  }
	}
//********************************************************************
//Determine if it is time to connect to the external logging server
//********************************************************************
	if(currentMillis - previousMillis_temp > interval_temp) { // READ ONLY ONCE PER INTERVAL
		previousMillis_temp = currentMillis; 
		if(debug==1){
			Serial.println(F("Temperature Times Up"));
		}

		if (client.connect(serverip,80)) { 
			Serial.println(F("InfluxDB Temperature Client Connected"));
			if (sesnor_name == 1) {
				client.print(F("GET /admin/server_add.php?"));
			}else if (sesnor_name == 2) {
				client.print(F("GET /admin/pit_add.php?"));
			}else if (sesnor_name == 3) {
				client.print(F("GET /admin/equipment_cabinet_add.php?"));
			}else if (sesnor_name == 4) {
				client.print(F("GET /admin/garage_add.php?"));
			}
			client.print(F("middle_sensor="));
			client.print(middle_sensor); 
			client.print(F("&left_sensor="));
			client.print(left_sensor); 
			if (sesnor_name == 4) {
				client.print(F("&right_sensor="));
				client.print(right_sensor); 
			}
			client.println(F(" HTTP/1.1"));
			client.print(F("Host: "));
			client.println(ip_to_str(serverip));
			client.println(F("Content-Type: application/x-www-form-urlencoded"));
			client.println(F("Connection: close"));
			client.println();
			client.println();
			client.println(F("Connection: close"));
			client.println();
			client.println();
			client.println(F("Connection: close"));
			client.println();
			client.println();
			client.stop();
			client.stop();
		} else{
			Serial.println(F("Temperature Logging Could Not Connect To Server"));
		}
    }

//********************************************************************
//Connect to health checks to ensure this arduino has not crashed
//********************************************************************
	if(currentMillis - previousMillis_heartbeat > interval_heartbeat) { // PERFORM ONLY ONCE PER INTERVAL
		previousMillis_heartbeat = currentMillis; 
		if(dnClient.getHostByName("hc-ping.com",healthcheckio_IP) == 1) {
			Serial.print(F("hc-ping.com = "));
			Serial.println(healthcheckio_IP);
		}else{
			Serial.print(F("DNS Lookup Failed"));
		}
		if(debug==1){
			Serial.println(F("Heartbeat Times Up"));
		}
		if (client.connect(healthcheckio_IP,80)) {
			Serial.println(F("Heartbeat Client Connected"));
			Serial.println("");
			client.print(F("GET /"));
			client.print(health_id);
			client.println(F(" HTTP/1.1"));
			client.println(F("Host: hc-ping.com"));
			client.println(F("Connection: close"));
			client.println();
			client.stop();
		} else{
			Serial.print(F("Heartbeat Could Not Connect To Server"));
		}
	}
}

//********************************************************************
//Function to get one-wire temp sensor (DS18S20) data
//********************************************************************
float getTemp(byte Sensor_PIN){
  OneWire  ds(Sensor_PIN); 
  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      Serial.print(F("Temperature Sensor on Pin "));
      Serial.print(Sensor_PIN);
      Serial.println(F(" - Could Not Be Located"));
      //return a really high specific number so we can tell something is wrong
      return 4096.0;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.print(F("Temperature Sensor on Pin "));
      Serial.print(Sensor_PIN);
      Serial.println(F(" - CRC is not valid!"));
      //return a really high specific number so we can tell something is wrong
      return 4096.0;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print(F("Temperature Sensor on Pin "));
      Serial.print(Sensor_PIN);
      Serial.println(F(" - Device Not Recognized!"));
      //return a really high specific number so we can tell something is wrong
      return 4096.0;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,0); // start conversion, without parasite power on at the end
  
  delay(1000); // Wait for temperature conversion to complete

  byte present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE); // Read Scratchpad

  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }
  
  ds.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16.0;
  

  if (temp_scale == 1){ //if using degrees F, the Celsius reading from the sensor must be converted
    TemperatureSum = (TemperatureSum * 1.8)+32.0;
  }
  return TemperatureSum;
}
