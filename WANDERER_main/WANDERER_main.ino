#include "ENITIO_enums.h"
#include "ENITIO_const.h"
#include "ENITIO_ir.h"
#include "ENITIO_joystick.h"
#include "ENITIO_buzzer.h"
#include "ENITIO_EspNOW.h"
#include "ENITIO_OLED.h"
#include "ENITIO_player_bluetooth.h"
#include "MainMenu.h"
#include "Profile.h"
#include "TreasureHuntPlayer.h"
#include "Admin.h"
#include "Credits.h"

TaskHandle_t backgroundTask;


void backgroundTaskCode(void * pvParameters){
  for ( ; ; ) {
    if (currentProcess == TreasureHuntProcess){
      if (!PLAYER.gameStarted){
        game_started_buffer = dbc.hasGameStarted();
      }
      else{
        PLAYER.gameBackgroundProcess();
      }
    }
    else{
      delay(50);
    }
  }
};

void setup() {
      // initialize serial
      Serial.begin(115200);
      EEPROM.begin(EEPROM_SIZE);

      // clearEEPROM();

      // check OLED display
      if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
      }

      StartUpDisplay();
      
      // initialize Joystick 
      Player_joystick.initialize();

      // initialize IR
      Player_IR.enable();
      
      Player_EspNOW.enable();

      dbc.connectToWiFi();
      my_MAC_address = WiFi.macAddress();

      GAME_CONSTANTS game_consts = dbc.getGameConstants();
      HTTP_TIMEOUT = game_consts.HTTP_TIMEOUT;
      EN_RECOVER_DURATION = game_consts.EN_RECOVER_DURATION;
      VIRUS_DECAY_DURATION = game_consts.VIRUS_DECAY_DURATION;
      VIRUS_IMMUNITY_DURATION = game_consts.VIRUS_IMMUNITY_DURATION;
      VIRUS_INFECTION_PROBABILITY = game_consts.VIRUS_INFECTION_PROBABILITY;
      PARTICIPANT_MaxHP = game_consts.PARTICIPANT_MaxHP;
      GL_MaxHP = game_consts.GL_MaxHP;
      PARTICIPANT_MaxEn = game_consts.PARTICIPANT_MaxEn;
      GL_MaxEn = game_consts.GL_MaxEn;
      INITIAL_MULTIPLIER = game_consts.INITIAL_MULTIPLIER;
      HEAL_MULTIPLIER = game_consts.HEAL_MULTIPLIER;
      MAX_ATTACK_MULTIPLIER = game_consts.MAX_ATTACK_MULTIPLIER;
      MAX_COLLECT_MULTIPLIER = game_consts.MAX_COLLECT_MULTIPLIER;
      BOMB_HP_DEDUCTION = game_consts.BOMB_HP_DEDUCTION;
      KILL_UPDATE_SERVER_INTERVAL = game_consts.KILL_UPDATE_SERVER_INTERVAL;

      Serial.println(my_MAC_address);

      pinMode(LED_BUILTIN, OUTPUT);

      xTaskCreatePinnedToCore(
                      backgroundTaskCode,   /* Task function. */
                      "backgroundTask",     /* name of task. */
                      10000,       /* Stack size of task */
                      NULL,        /* parameter of the task */
                      2,           /* priority of the task */
                      &backgroundTask,      /* Task handle to keep track of created task */
                      0);
}

void loop() {
  // First check if ESP is connected to WiFi
  if ((WiFi.status() != WL_CONNECTED) && (millis() - last_disconnected_time > 2000)) {
    Serial.println("Lost WiFi Connection.. attempting to reconnect");
    digitalWrite(LED_BUILTIN, HIGH); // lights up the built-in LED when the WiFi connection is lost
    dbc.startWiFiConnection();
    last_disconnected_time = millis();
  }
  if (currentProcess == MainMenuProcess){
    My_MainMenu.MainMenuLoop();
  }
  else if (currentProcess == TreasureHuntProcess){
    PLAYER.gameMainLoop();
  }
  else if (currentProcess == ProfileProcess){
    My_Profile.ProfileLoop();
  }
  else if (currentProcess == AdminProcess){
    My_Admin.AdminLoop();
  } else if (currentProcess == CreditProcess) {
    My_Credits.CreditsLoop();
  }
  else {
    currentProcess = MainMenuProcess;
  }
}
