#include "ENITIO_wifi.h"

typedef struct feedback_message {
  int msg_type = 1;
  int attackee_type; // 1 for player, 2 for Lvl 1 Treasure, 3 for Lvl 2 Treasure
  int attacker_CLAN;
  int attacker_ID; 
  int attackee_CLAN; // or Treasure ID
  bool is_attackee_killed;
  int powerup_received;
} feedback_message;

int EspNOW_received = 0;
feedback_message EspNOW_recvData;
bool last_send_status = true;

class EspNOW {
  private:
    feedback_message feedbackData;
    esp_now_peer_info_t peerInfo;
    uint8_t broadcastAddress[6];
    
    
  public:
    int is_waiting_for_feedback = 0;
    
    void enable() {
      // WiFi.mode(WIFI_STA);
      WiFi.mode(WIFI_AP_STA);
      if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
      }
      Serial.println("OK");
      esp_now_register_send_cb(OnDataSent);
      // esp_now_register_recv_cb(OnDataRecv);
    }

    void getDeviceMACAddress(int attacker_CLAN, int attacker_ID){
      broadcastAddress[0] = 4;
      broadcastAddress[1] = 8;
      broadcastAddress[2] = 1;
      broadcastAddress[3] = attacker_CLAN;
      broadcastAddress[4] = attacker_ID;
      broadcastAddress[5] = 1;
//
//      broadcastAddress[0] = 255;
//      broadcastAddress[1] = 255;
//      broadcastAddress[2] = 255;
//      broadcastAddress[3] = 255;
//      broadcastAddress[4] = 255;
//      broadcastAddress[5] = 255;
    }
  
   void send_data(int attackee_type, int attacker_CLAN, int attacker_ID, int attackee_CLAN, int powerup_id){
      // Register peer
      feedbackData.attackee_type = attackee_type;
      feedbackData.attacker_CLAN = attacker_CLAN;
      feedbackData.attacker_ID = attacker_ID;
      feedbackData.attackee_CLAN = attackee_CLAN;
      feedbackData.is_attackee_killed = false;
      feedbackData.powerup_received = powerup_id;
      
      memcpy(peerInfo.peer_addr, broadcastAddress, 6);
      peerInfo.channel = 0;  
      peerInfo.encrypt = false;
        
      // Add peer     
      if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("Failed to add peer");
        return;
      }
      
      // Send message via ESP-NOW
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &feedbackData, sizeof(feedbackData));
       
      if (result == ESP_OK) {
        Serial.println("Sent with success");
      }
      else {
        Serial.println("Error sending the data");
      }
      
      esp_now_del_peer(broadcastAddress);
    }

    void disable(){
      esp_now_deinit();
    } 

    static void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
       memcpy(&EspNOW_recvData, incomingData, sizeof(EspNOW_recvData));
       EspNOW_received = 1;
    }

    static void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
      Serial.print("WiFi Channel: "); Serial.println(WiFi.channel());
      Serial.print("\r\nLast Packet Send Status:\t");
      if (status == ESP_NOW_SEND_SUCCESS){
        Serial.println("Delivery Success"); }
      else {
          Serial.println("Delivery Failed");
        // failed_kill_feedback ++ ;
        // failed_kill_CLAN[current_failed_save_pointer] = mac_addr[3];
        // failed_kill_ID[current_failed_save_pointer] = mac_addr[4];
        // current_failed_save_pointer ++ ;
        // if(current_failed_save_pointer >= 50) current_failed_save_pointer -= 50;
        }      
     // last_send_status = (status == ESP_NOW_SEND_SUCCESS);
    }

    feedback_message get_feedback_received(){
      return EspNOW_recvData;
    }
};

EspNOW TreasureLevel1_EspNOW;
