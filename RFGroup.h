#include "RF24.h"
#include "CommandProvider.h"

#pragma pack(push, 1) // prevents memory alignment from disrupting the layout and size of the network packet 
struct SyncPacket {
  uint16_t groupID;
  uint32_t padding;
  uint8_t lfo[4];
  uint8_t global_hue;
  uint8_t global_sat;
  uint8_t global_val;
  uint8_t global_speed;
  uint8_t global_density;
  uint8_t lfo_active : 1;
  uint8_t hue_active : 1;
  uint8_t sat_active : 1;
  uint8_t val_active : 1;
  uint8_t speed_active : 1;
  uint8_t density_active : 1;
  uint8_t reserved[2];
  uint8_t page;
  uint8_t mode;
  uint8_t adjust_active : 1;
  uint8_t wakeup : 1;
  uint8_t power_off : 1;
  uint8_t force_reload : 1;
  uint8_t save : 1;
  uint8_t _delete : 1;
  uint8_t randomize_adjust : 1;
  uint8_t alternate : 1;
};
#pragma pack(pop)

class RFGroup
{
  public:
    RFGroup() : isDirty(true) {}

    ~RFGroup() {}

    int dirtyCount = 5;
    bool isDirty;

    void setup(int gid, RF24 * r)
    {
      groupID = gid;
      radio = r;
      packet.padding = 1;
      packet.groupID = ((gid & 0xff) << 8) | ((gid >> 8) & 0xff);
    }

    RF24 * radio = nullptr;
    int groupID = -1;
    SyncPacket packet;
    
    void sendPacket(bool force = false) {
      if(groupID <= 0) return;
      if(dirtyCount == 0 && !force) return;
      
      DBG("SENDING PACKET: (adjust active: " + String(packet.adjust_active));
      print_bytes(&packet, sizeof(SyncPacket));

      radio->write(&packet, sizeof(SyncPacket));
      
      if(!force) dirtyCount = max(dirtyCount -1, 0);
    }
    
    void setData(CommandProvider::PatternData data, bool doNotUpdateIfSame = false)
    {
      DBG("setData()");
      if(doNotUpdateIfSame)
      {
        
        if(packet.page == data.page
        && packet.mode == data.mode
 /*       && packet.lfo_active == data.actives & 1//true;
        && packet.hue_active == (data.actives >> 1) & 1//true;
        && packet.sat_active == (data.actives >> 2) & 1//true;
        && packet.val_active == (data.actives >> 3) & 1//true;
        && packet.speed_active == (data.actives >> 4) & 1//true;
        && packet.density_active == (data.actives >> 5) & 1//true;*/
        && packet.lfo[0] == data.lfo1
        && packet.lfo[1] == data.lfo2
        && packet.lfo[2] == data.lfo3
        && packet.lfo[3] == data.lfo4
        && packet.global_val == data.brightness
        && packet.global_hue == data.hueOffset
        && packet.global_sat == data.saturation
        && packet.global_speed == data.speed
        && packet.global_density == data.density
        && packet.adjust_active == data.adjust_active
        ) {
          //DBG(String(groupID) +  " : same packet");
          return;
        }else {
        }
      }

      packet.padding++;
      dirtyCount = 5;
      
      packet.page = data.page;
      packet.mode = data.mode;
      packet.wakeup = false;
      packet.power_off = false;


      packet.lfo_active = data.actives & 1;//true;
      packet.hue_active = (data.actives >> 1) & 1;//true;
      packet.sat_active = (data.actives >> 2) & 1;//true;
      packet.val_active = (data.actives >> 3) & 1;//true;
      packet.speed_active = (data.actives >> 4) & 1;//true;
      packet.density_active = (data.actives >> 5) & 1;//true;

      DBG("SET PACKET ON GROUP: " + String(data.adjust_active));

      packet.adjust_active = data.adjust_active;
      packet.randomize_adjust = data.randomize_adjust;

      packet.lfo[0] = data.lfo1;
      packet.lfo[1] = data.lfo2;
      packet.lfo[2] = data.lfo3;
      packet.lfo[3] = data.lfo4;
      packet.global_val = data.brightness;
      packet.global_hue = data.hueOffset;
      packet.global_sat = data.saturation;
      packet.global_speed = data.speed;
      packet.global_density = data.density;
/*
      DBG("Set Pattern, groupID = "+String(groupID)+", padding = "+packet.padding
      +", LFO : "+String(packet.lfo_active)+" > lfo0 : "+String(packet.lfo[0])
      +", Hue active : "+String(packet.hue_active)+" > hue : "+String(packet.global_hue)+", sat : "+String(packet.global_sat)+", val : "+String(packet.global_val)
      +", speed : "+String(packet.global_speed)+", density : "+String(packet.global_density)
      );
*/  
    }

     bool updateFromPacket(SyncPacket receivingPacket) {
      if(receivingPacket.padding > packet.padding) {
         packet.padding = receivingPacket.padding;
         return true;
      } else if (packet.power_off != receivingPacket.power_off) {
        packet.power_off = receivingPacket.power_off;
        packet.padding = 0;
        return true;
      }


      return false;
      
      //packet.page = receivingPacket.page;
      //packet.mode = receivingPacket.mode;

      //DBG("LFO Active : "+String(receivingPacket.lfo_active));
      //DBG("Global Active : "+String(receivingPacket.global_active));
      //DBG("Hue : "+String(receivingPacket.global_hue));
      //DBG("Sat : "+String(receivingPacket.global_sat));
      //DBG("Val : "+String(receivingPacket.global_val));
      //DBG("LFO 0 : "+String(receivingPacket.lfo[0]));
           
/*
      packet.lfo_active = true;
      packet.global_active = true;
      packet.lfo[0] = data.lfo1;
      packet.lfo[1] = data.lfo2;
      packet.lfo[2] = data.lfo3;
      packet.lfo[3] = data.lfo4;
      packet.global_intensity = data.brightness;
      packet.global_hue = data.hueOffset;
      packet.global_sat = data.saturation;
      packet.global_palette = 0;
      packet.global_speed = data.speed;
      packet.global_density = data.density;
      */
     }

     void wakeUp()
     {
        packet.padding++;
        packet.wakeup = true;
        dirtyCount = 10;
     }

     void powerOff()
     {
        packet.padding++;
        packet.power_off = true;
        dirtyCount = 10;
     }
};
