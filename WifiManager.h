#pragma once

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <math.h>  

#include "Config.h"
#include "SerialManager.h"

#define CONNECT_TIMEOUT 5000
#define CONNECT_TRYTIME 2000

class WifiManager
{
public:
  WifiManager(){  
      setCallbackConnectionUpdate(&WifiManager::connectionUpdateDefaultCallback);
  }
  
  ~WifiManager(){}

  bool broadcastingLocal = false;
  bool isConnected = false;
  bool isConnecting = false;
  bool hasScannedForNetworks = false;
  
  String ssid;
  String pass;
  
  long timeAtStartConnect;
  long timeAtLastConnect;

  bool isActivated;
  
  void init()
  {
    // isActivated = true; //Config::instance->getWifiMode();
    //
    // if(isConnected)
    // {
    //   DBG("Disconnecting first...");
    //   WiFi.disconnect();
    //   delay(100);
    // }
    //
    // if(!isActivated)
    // {
    //   DBG("Wifi is not activated, not initializing");
    //   return;
    // }
    //
    //
    // digitalWrite(13, HIGH);
  }

  void update() {
    // if(!isActivated) return;
    // if (!broadcastingLocal) setupLocal();
    //
    // if(!isConnected)
    //   connectToNetwork();
  }

  void attemptToConnect(String ssid) {
    String password = Config::instance->getWifiPassword(ssid);

    if (password.length() > 0) {
      DBG("Connecting to Wifi "+ssid+" with password "+pass);
      if (isConnecting)
        networksToTryLater.push_back(ssid);
      else {
        isConnecting = true;
        setConnected(false);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());
        timeAtStartConnect = millis();
        timeAtLastConnect = millis();
      }
    }
  }

  std::vector<String> networksToTryLater;

	const char* wl_status_to_string(wl_status_t status) {
		switch (status) {
			case WL_NO_SHIELD: return "[NO SHIELD]";
			case WL_IDLE_STATUS: return "[IDLE STATUS]";
			case WL_NO_SSID_AVAIL: return "[NO SSID AVAIL]";
			case WL_SCAN_COMPLETED: return "[SCAN COMPLETED]";
			case WL_CONNECTED: return "[CONNECTED]";
			case WL_CONNECT_FAILED: return "[CONNECT FAILED]";
			case WL_CONNECTION_LOST: return "[CONNECTION LOST]";
			case WL_DISCONNECTED: return "[DISCONNECTED]";
		}
	}

  void connectToNetwork() {
    if(millis() > timeAtLastConnect + CONNECT_TRYTIME) {
      DBG("CONNECTING TO WIFI " + String(wl_status_to_string(WiFi.status())));
      timeAtLastConnect = millis();
      if(WiFi.status() == WL_CONNECTED) {  
       digitalWrite(13, LOW);

				DBG("Grabbing local IP address:");
				String localIp = WiFi.localIP().toString();
        DBG("WiFi Connected, local IP : "+localIp);

        // broadcastingLocal = false;
        isConnecting = false;
        setConnected(true);
    
         return;
      } else {
        setConnected(false);

				if (networksToTryLater.size() > 0) {
					std::vector<String> networksToTry = networksToTryLater;
					networksToTryLater = {};
					for (int i = 0; i < networksToTry.size(); ++i)
						attemptToConnect(networksToTry[i]);
				} else if (!hasScannedForNetworks) {
					attemptToConnect(Config::instance->getMostRecentWifiSSID());

					int foundNetworks = WiFi.scanNetworks();
					hasScannedForNetworks = true;
					if (foundNetworks > 0) {
						Serial.print(foundNetworks);
						Serial.println(" networks found");
						for (int i = 0; i < foundNetworks; ++i)
							attemptToConnect(WiFi.SSID(i));
					}
				}
			}
    }
        
    // ????????? I don't know what this does,
    //
    // Maybe it tries to re-broadcast to wifi to keep it active?
    // Do we need it?
    //
    // if(millis() > timeAtStartConnect + CONNECT_TIMEOUT) {
    //   // DBG("Could not connect to "+ssid);
    //   setConnected(false);
    //   for(int i=0;i<5;i++) {
    //     digitalWrite(13, HIGH);
    //     delay(50);
    //     digitalWrite(13, LOW);
    //     delay(50);
    //   }
    // }
    //   
    //   // setupLocal();
    // }
  }

  void setupLocal() {
    DBG("SETTING UP LOCAL WIFI NETWORK:");
    String softAPName = Config::instance->getDeviceName();
    String softAPPass = "findyourflow";
    WiFi.softAP(softAPName.c_str(), softAPPass.c_str());
    DBG("Local IP : "+String(WiFi.softAPIP()[0])+
    "."+String(WiFi.softAPIP()[1])+
    "."+String(WiFi.softAPIP()[2])+
    "."+String(WiFi.softAPIP()[3]));

    broadcastingLocal = true;
    onConnectionUpdate();
    
    DBG("AP WiFi is init!!");
    DBG("AP WiFi is init!! name:" + String(softAPName));
    DBG("AP WiFi is init!! pass:" + String(softAPPass));
  }

  void setConnected(bool value) {
    isConnected = value;
    onConnectionUpdate();
  }


  typedef void(*onConnectionUpdateEvent)();
    void (*onConnectionUpdate) ();

    void setCallbackConnectionUpdate (onConnectionUpdateEvent func) {
      onConnectionUpdate = func;
    }

    static void connectionUpdateDefaultCallback()
    {
      //nothing
    }
};
