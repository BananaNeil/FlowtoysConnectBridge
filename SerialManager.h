#pragma once

#include "CommandProvider.h"

#if SERIAL_DEBUG
#define DBG(msg) Serial.println(msg)
#else
#define DBG(msg)
#endif

class SerialManager :
  public CommandProvider
{
  public:
    SerialManager() :
      CommandProvider("Serial")
    {
      instance = this;
    }

    ~SerialManager() {}

    static SerialManager * instance;
    char buffer[256];
    int bufferIndex = 0;

    void init()
    {
      Serial.begin(115200);
      memset(buffer, 0, 256);
    }

    void update()
    {
      while (Serial.available())
      {
        byte c = Serial.read();
        //DBG("Got char : "+String(c));
        if (c == 255 || c == '\n')
        {
          parseMessage(buffer);
          memset(buffer, 0, 255);
          bufferIndex = 0;
        } else
        {
          if (bufferIndex < 255) buffer[bufferIndex] = c;
          bufferIndex++;
        }
      }
    }

    void parseMessage(String message)
    {
      char command = message[0];
      DBG("Parse Message,command is : " + String(command));

      switch (command)
      {
        case 's':
          {
            CommandData d;

            d.type = SYNC_RF;

            String timeout = message.substring(1,message.length());
            DBG("Timeout : "+timeout);
            d.value1.floatValue = timeout.toFloat();
            sendCommand(d);
          }
          break;

        case 'S': sendCommand(STOP_SYNC); break;
        case 'a':
          {
            sendCommand(RESET_SYNC);
          }
          break;
          
        case 'n':
          {
            CommandData d;
            d.type = SET_WIFI_CREDENTIALS;
            String split[2];
            int num = splitString(&message[1], split, 2);
            if(num >= 2)
            {
              d.value1.stringValue = (char *)split[0].c_str();
              d.value2.stringValue = (char *)split[1].c_str();
              sendCommand(d);
            }
           
          }
          break;
          
        case 'g':
          {
            DBG("SET GLOBAL CONFIG");
            CommandData d;
            d.type = SET_GLOBAL_CONFIG;
            String split[2];
            DBG("MESSAGE: " + String(message));
            int num = splitString(&message[1], split, 2);
            if(num > 0) d.value1.stringValue = (char *)split[0].c_str();
            d.value2.intValue = num > 1?split[1].toInt():2;
            sendCommand(d);
          }
          break;

        case 'w':
        case 'W':
        case 'z':
        case 'Z':
          {
            CommandData d;
            d.type = (command == 'w' || command == 'W') ? WAKEUP : POWEROFF;
            String split[1];
            int num = splitString(&message[1], split, 1);
            d.value1.intValue = num > 0?split[0].toInt():0; //group id
            d.value2.intValue = (command == 'W' || command == 'Z'); //public with capital
            sendCommand(d);

          }
          break;

        case 'r':
        {
          ESP.restart();
        }
        break;
        case 'R': {
          conf.hardReset();
          ESP.restart();
        }
        break;
        
        case 'p':
        case 'P':
          {
            PatternData p;
            String split[14];
            int num = splitString(&message[1], split, 14);
            
            if(num > 0) p.groupID = split[0].toInt();
            p.groupIsPublic = command == 'P';

            if(num > 1) p.page = split[1].toInt();
            if(num > 2) p.mode = split[2].toInt();

            if(num > 3) p.actives = split[3].toInt();

            if(num > 4) p.hueOffset = split[4].toInt();
            if(num > 5) p.saturation = split[5].toInt();
            if(num > 6) p.brightness = split[6].toInt();
            if(num > 7) p.speed = split[7].toInt();
            if(num > 8) p.density = split[8].toInt(); 

            if(num > 9) p.lfo1 = split[9].toInt();
            if(num > 10) p.lfo2 = split[10].toInt();
            if(num > 11) p.lfo3 = split[11].toInt();
            if(num > 12) p.lfo4 = split[12].toInt();
            if(num > 13) p.adjust_active = split[13].toInt() & 1;
            if(num > 13) p.randomize_adjust = (split[13].toInt() & 64) == 64;

            DBG("PATTERN DATA (from parse): ");
            // print_bytes(&p, sizeof(PatternData));
            DBG("=== " + String(p.randomize_adjust) );
            DBG("===== " + String(split[13].toInt()) );

            sendPattern(p);
          }
          break;
      }
    }

    int splitString(char * source, String * dest, int maxNum)
    {
      char * pch = strtok (source, ",");
      int i = 0;
      while (pch != NULL && i < maxNum)
      {
        dest[i] = String(pch);
        pch = strtok (NULL, ",");
        i++;
      }

      return i;
    }

		void sendCommandFromRF(String command) {
			DBG("Send command from RF to Serial: " + command);
		}

    void sendTrigger(String name)
    {
      Serial.println(name);
    }

    void sendBoolValue(String name, bool value)
    {
      Serial.println(name + " " + (value ? 1 : 0));
    }

    void sendIntValue(String name, int value)
    {
      Serial.println(name + " " + String(value));
    }

    void sendFloatValue(String name, float value)
    {
      Serial.println(name + " " + String(value));
    }

};

SerialManager * SerialManager::instance = nullptr;
