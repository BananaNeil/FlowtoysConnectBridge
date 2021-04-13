#pragma once

#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>
#include <ESPmDNS.h>
#include "SerialManager.h"


class OSCManager :
  public CommandProvider {
public:
  OSCManager() : CommandProvider("OSC") {}
  ~OSCManager(){}
    
  WiFiUDP Udp;
  const unsigned int localPort = 9000;        // local port to listen for UDP packets (here's where we send the packets)
  char address[64];
   
  void init() { 

    // DBG("Init OSC");
    //
    // Udp.stop();
    // Udp.begin(localPort);
    //
    // String deviceName = Config::instance->getDeviceName();
    // if (!MDNS.begin(deviceName.c_str())) {
    // // if (!MDNS.begin("N FlowConnect")) {
    //     DBG("Error setting up MDNS responder!");
    // } else {
    //     MDNS.addService("_osc", "_udp", localPort);
    //     DBG("_osc._udp. "+ String(deviceName) + " zeroconf is init.");
    // }
    //
    // DBG("OSC Initialized");// listening on "+String(buf)+":"+String(localPort));
  }

  char packetBuffer[255]; //buffer to hold incoming packet

  IPAddress remoteIp;
  int remotePort;

  void sendMessage(char * path, void *bytePointer, size_t size) {
    DBG("REMOTE IP: " + String(remoteIp));
    if (!remoteIp) return;
    OSCMessage msg(path);

    uint8_t *bytes = new uint8_t[size+1];
    bytes = (unsigned uint8_t*)bytePointer;
    for (int i = 0; i<size; i++)
      msg.add(bytes[i]);

    for (int i = 0; i < msg.size(); ++i) {
      // if (msg.isString(i)) {
        char str[255];
        msg.getString(i, str, 255);
        Serial.println(str);
      // }
    }


    Udp.beginPacket(remoteIp, remotePort);
    msg.send(Udp); // send the bytes to the SLIP stream
    Udp.endPacket(); // mark the end of the OSC Packet
    msg.empty(); // free space occupied by message
  }
  
  void update() {
		// int packetSize = Udp.parsePacket();
    //
		// if (packetSize) {
		// 	Serial.print("Received packet of size ");
		// 	Serial.println(packetSize);
		// 	Serial.print("From ");
			// IPAddress remoteIp = Udp.remoteIP();
		// 	Serial.print(remoteIp);
		// 	Serial.print(", port ");
		// 	Serial.println(Udp.remotePort());
		// 	// read the packet into packetBufffer
    //
		// 	int len = Udp.read(packetBuffer, 255);
    //
		// 	if (len > 0) {
		// 		packetBuffer[len] = 0;
		// 	}
    //
		// 	Serial.println("Contents:" + String(len));
		// 	Serial.println(packetBuffer);
		// }




    // update2();
  }


  void update2() {
    int size = Udp.parsePacket();
    if (size > 0) {
      int originalSize = size;
      DBG("Size of message: " + String(size));
      OSCMessage msg;
      while (size--) {
        msg.fill(Udp.read());
      }

      remoteIp = Udp.remoteIP();
      remotePort = Udp.remotePort();


      for (int i = 0; i < 4; i++) {
        Serial.print(remoteIp[i], DEC);
        if (i < 3) {
          Serial.print(".");
        }
      }
      DBG("\nREMOTE IP ^ Port: "+ String(remotePort));

      Serial.println("message received!");
      for (int i = 0; i < msg.size(); ++i) {
        DBG("IS STRING? " + String(msg.isString(i)));
      	if (msg.isString(i)) {
      		char str[255];
      		msg.getString(i, str, 255);
      		Serial.println(str);
      	}
      }


      if (!msg.hasError()) {
        DBG("Received message: NO ERROR");
        // DBG("Pattern: " + String(msg.getAddressPattern().toString()));
        if(msg.fullMatch("/wakeUp")) {
           CommandData d;
            d.type = WAKEUP;
            msg.getString(0,d.value1.stringValue);
            msg.getString(1,d.value2.stringValue);
           d.value1.intValue = msg.size() > 0 ? msg.getInt(0) : 0; //group
           d.value2.intValue = msg.size() > 1 ? msg.getInt(1) : 0; //isPublic
           sendCommand(d);
        } else if (msg.fullMatch("/powerOff")) {
            CommandData d;
            d.type = POWEROFF;
            msg.getString(0,d.value1.stringValue);
            msg.getString(1,d.value2.stringValue);
           d.value1.intValue = msg.size() > 0 ? msg.getInt(0) : 0; //group
           d.value2.intValue = msg.size() > 1 ? msg.getInt(1) : 0; //isPublic
           sendCommand(d);
        } else if (msg.fullMatch("/sync")) {
            CommandData d;
            d.type = SYNC_RF;
            d.value1.floatValue =  msg.size() > 0 ? msg.getFloat(0) : 0;
           sendCommand(d);
        } else if (msg.fullMatch("/stopSync")) {
           sendCommand(STOP_SYNC);
        } else if (msg.fullMatch("/resetSync")) {
          sendCommand(RESET_SYNC);
        } else if (msg.fullMatch("/pattern")) {
          DBG("PATTERN COMMAND " + String(msg.size()));
          PatternData p;

          // uint16_t are 5 chars long
					char groupID[255];
					if (msg.isString(0))
						msg.getString(0, groupID, 255);
          DBG("Now, turn it into an int" + String(groupID));
          p.groupID = to_int(groupID);

          // uint16_t groupID = msg.getString(0);
          // groupID = (groupID >> 8 & 0xff) | ((groupID & 0xff) << 8);
          DBG("FIRST THING FIRST: "+ String(p.groupID));
          // p.groupID = groupID;
          p.groupIsPublic = msg.getInt(1);
          p.page = msg.getInt(2);
          p.mode = msg.getInt(3);

          p.actives = msg.getInt(4);

          p.hueOffset = msg.getInt(5);
          p.saturation = msg.getInt(6);
          p.brightness = msg.getInt(7);
          p.speed = msg.getInt(8);
          p.density = msg.getInt(9);

          p.lfo1 = msg.getInt(10);
          p.lfo2 = msg.getInt(11);
          p.lfo3 = msg.getInt(12);
          p.lfo4 = msg.getInt(13);
          p.adjust_active = msg.getInt(14) & 1;
          p.randomize_adjust = msg.getInt(14) & 64;

         DBG("RECEIVED PATTERN with adjust: " + String(msg.getInt(14)) +" setting it to: " + String(p.adjust_active));
         // print_bytes(&p, sizeof(PatternData));
          sendPattern(p);
        } else if(msg.fullMatch("/factory-reset")) {
          conf.hardReset();
          ESP.restart();
        } else if(msg.fullMatch("/wifiSettings")) {
          DBG("RECEIVED WIFI CREDENTILAS; ");
          CommandData d;
          d.type = SET_WIFI_CREDENTIALS;
					char network[255];
					char password[255];
					if (msg.isString(0))
						msg.getString(0, network, 255);
					if (msg.isString(1))
						msg.getString(1, password, 255);

					d.value1.stringValue = network;
					d.value2.stringValue = password;
					sendCommand(d);
        } else if(msg.fullMatch("/setNetworkName")) {
            CommandData d;
            d.type = SET_GLOBAL_CONFIG;
            char networkName[255];
            DBG("Recieved network name!");
            if (msg.isString(0))
              msg.getString(0, networkName);

            d.value1.stringValue = networkName;
            // d.value2.intValue = msg.size() > 1?msg.getInt(1):2; //0 is wifi, 1 is BLE, 2 is both
            DBG("Recieved network name! ...... ");
            sendCommand(d);
        } else if(msg.fullMatch("/play")) {
           CommandData d;
            d.type = PLAY_SHOW;
            msg.getString(0,d.value1.stringValue);
            sendCommand(d);
        } else if(msg.fullMatch("/stop")) {
           sendCommand(STOP_SHOW);
        } else if(msg.fullMatch("/pause")) {
           sendCommand(PAUSE_SHOW);
        } else if(msg.fullMatch("/resume")) {
           sendCommand(RESUME_SHOW);
        } else if(msg.fullMatch("/seek")) {
           CommandData d;
            d.type = SEEK_SHOW;
            d.value1.floatValue = msg.getFloat(0);
            sendCommand(d);


        /*else if(msg.fullMatch("/group"))
        {
           CommandData d;
            d.type = SET_GROUP;
            d.value1.intValue = msg.getInt(0);
            sendCommand(d);
        }else if(msg.fullMatch("/page"))
        {
          CommandData d;
            d.type = SET_PAGE;
            d.value1.intValue = msg.getInt(0);
            sendCommand(d);
        }else if(msg.fullMatch("/setAll"))
        {
          CommandData d;
            d.type = SET_ALL;
            d.value1.intValue = msg.getInt(0);
            d.value2.intValue = msg.getInt(1);
            d.value3.intValue = msg.getInt(2);
            sendCommand(d);
        }else if(msg.fullMatch("/pageMode"))
        {
          CommandData d;
            d.type = SET_PAGEMODE;
            d.value1.intValue = msg.getInt(0);
            d.value2.intValue = msg.getInt(1);
            sendCommand(d);
        }else if (msg.fullMatch("/mode"))
        {
         CommandData d;
            d.type = SET_MODE;
            d.value1.intValue = msg.getInt(0);
            sendCommand(d);
        }else if(msg.fullMatch("/nextPage"))
        {
          sendCommand(NEXT_PAGE);
        }else if(msg.fullMatch("/nextMode"))
        {
          sendCommand(NEXT_MODE);
        } else if(msg.fullMatch("/lfo"))
        {
           CommandData d;
            d.type = SET_LFO;
            d.value1.intValue = msg.getInt(0);
            d.value2.intValue = msg.getInt(1);
            sendCommand(d);
        } else if(msg.fullMatch("/intensity"))
        {
           CommandData d;
            d.type = SET_INTENSITY;
            d.value1.intValue = msg.getInt(0);
            sendCommand(d);
        } else if(msg.fullMatch("/hsv"))
        {
           CommandData d;
            d.type = SET_HSV;
            d.value1.intValue = msg.getInt(0);
            d.value2.intValue = msg.getInt(1);
            d.value3.intValue = msg.getInt(2);
            sendCommand(d);
        } else if(msg.fullMatch("/speedDensity"))
        {
           CommandData d;
            d.type = SET_SPEEDDENSITY;
            d.value1.intValue = msg.getInt(0);
            d.value2.intValue = msg.getInt(1);
            sendCommand(d);
        } else if(msg.fullMatch("/palette"))
        {
           CommandData d;
            d.type = SET_PALETTE;
            d.value1.intValue = msg.getInt(0);
            sendCommand(d);
        }else if(msg.fullMatch("/sync"))
        {
           sendCommand(SYNC_RF);
        }*/

        #if USE_STREAMING
        } else if(msg.fullMatch("/rgb/brightness")) {
          globalBrightness = msg.getFloat(0);
       #endif

       } else {
          char addr[32];
          msg.getAddress(addr, 0);
          DBG("OSC Address not handled : "+String(addr));
        }
      }
    }
  }
};
