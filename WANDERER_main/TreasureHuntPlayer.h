#define ACTION_RECV_WAIT 150 // [ms] 
#define TEMP_NOTI_WAIT 6000 // including being Attacked, Healed, Successfully Attack and Successfully Collect
#define NOTI_SOUND_DURATION 200
#define TEMP_NOTI_BLINKING 300
#define FEEDBACK_WAIT 5000

#define LUCKY_NOT_INFECTED_DURATION 20000 // [ms]

#define x2_En_Regen_bonus_duration 300000 // [ms]

int game_started_buffer = 0;

uint8_t newMACAddress_AP[] = {4, 8, 1, 255, 255, 0};
uint8_t newMACAddress_STA[] = {4, 8, 1, 255, 255, 1};

class TreasureHuntPlayer
{
  private:
    int ID;
    int CLAN;
    bool _isGL;
    int HP;
    int MaxHP;
    int En; // energy
    int MaxEn;
    int Multiplier;

    action_id action;
    bool poisonActive = false;

    unsigned long lastActionReceived = 0;

    int currentPage = 0;
    int lastPageNav = 0;

    String tempNoti = "";
    String permNoti = "";

    unsigned long Nav_start = 0;
    unsigned long tempNoti_start = 0;
    unsigned long last_max_en_decay = 0;
    unsigned long last_en_recover = 0;
    unsigned long last_hp_decay = 0;
    unsigned long last_received_heal = 0;
    unsigned long last_lucky_not_infected = 0;
    unsigned long start_x2_en_regen = 0;
    unsigned long last_update_kills_to_server = 0;

    unsigned long start_receiving_feedback = 0;

    int numKilled = 0;
    int numL1Treasure = 0;
    int numL2Treasure = 0;

    int num_bonus6HP = 0;
    int num_bonus1MaxEn = 0;
    int num_bonus1Multiplier = 0;
    int num_fiveminx2EnRegen = 0;
    int num_bomb = 0;
    int num_poison = 0;

    bool is_x2EnRegen = false;

    int PowerUpNav = 0;
    bool choosingPowerUp = false;

    bool active_bomb = 0;

    int temp_bomb_attacked = 0;
    int temp_bomb_killed = 0;

  public:
    bool gameStarted = 0;
    bool deviceReady = 0;

    void setup_initial_state(int id, int clan, bool isGL) {
      ID = id;
      CLAN = clan;
      _isGL = isGL;

      newMACAddress_AP[3] = CLAN;
      newMACAddress_AP[4] = ID;

      newMACAddress_STA[3] = CLAN;
      newMACAddress_STA[4] = ID;

      esp_wifi_set_mac(WIFI_IF_AP, &newMACAddress_AP[0]);
      esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress_STA[0]);


      Serial.print("[NEW] ESP32 Board MAC Address:  ");
      Serial.println(WiFi.macAddress());

      if (EEPROM.read(PLAYER_enable_add) == 0) {
        if (!_isGL) {
          HP = PARTICIPANT_MaxHP;
          En = PARTICIPANT_MaxEn;
          MaxHP = PARTICIPANT_MaxHP;
          MaxEn = PARTICIPANT_MaxEn;
          Multiplier = INITIAL_MULTIPLIER;
        }
        else {
          HP = GL_MaxHP;
          En = GL_MaxEn;
          MaxHP = GL_MaxHP;
          MaxEn = GL_MaxEn;
          Multiplier = INITIAL_MULTIPLIER;
        }

        EEPROM.write(PLAYER_enable_add, 1);
        EEPROM.write(PLAYER_HP_add, HP);
        EEPROM.write(PLAYER_EN_add, En);
        EEPROM.write(PLAYER_MaxHP_add, MaxHP);
        EEPROM.write(PLAYER_MaxEn_add, MaxEn);
        EEPROM.write(PLAYER_MULTIPLIER_add, Multiplier);
        EEPROM.commit();
      }
      else {
        HP = EEPROM.read(PLAYER_HP_add);
        En = EEPROM.read(PLAYER_EN_add);
        MaxHP = EEPROM.read(PLAYER_MaxHP_add);
        MaxEn = EEPROM.read(PLAYER_MaxEn_add);
        Multiplier = EEPROM.read(PLAYER_MULTIPLIER_add);
        numKilled = EEPROM.read(PLAYER_numKilled_add);
        numL1Treasure = EEPROM.read(PLAYER_numL1Treasure_add);
        numL2Treasure = EEPROM.read(PLAYER_numL2Treasure_add);
        num_bonus6HP = EEPROM.read(PLAYER_num_bonus6HP_add);
        num_bonus1MaxEn = EEPROM.read(PLAYER_num_bonus1MaxEn_add);
        num_bonus1Multiplier = EEPROM.read(PLAYER_num_bonus1MULTIPLIER_add);
        num_fiveminx2EnRegen = EEPROM.read(PLAYER_num_fiveminx2EnRegen_add);
        num_bomb = EEPROM.read(PLAYER_num_bomb_add);
        num_poison = EEPROM.read(PLAYER_num_poison_add);
      }

      if (WIFI_ON) WiFi.disconnect();
      // change to correct channel, according to clan
      int clan_channel = dbc.getClanWiFiChannel(CLAN);
      Serial.print("[INITIALISE] Changing to WiFi "); Serial.println(clan_channel);
      dbc.changeWiFiChannel(clan_channel);
      Player_EspNOW.enable();
    }

    void sendAction() {
      // format of the IR signal (16-bit hexadecimal, i.e. 4 digits)
      // address: 0x0<CLAN><ID - 2 bit>  (ID is 2 bits as there maybe more than 16 people in one CLAN)
      // command: 0x0<Current WiFi Channel><MULTIPLIER><Action>
      if ((action != do_nothing) && (En > 0)) {
        uint16_hex_digits address_digits, command_digits;

        address_digits.digit0 = ID % 16;
        address_digits.digit1 = ID / 16;
        address_digits.digit2 = CLAN;

        command_digits.digit2 = WiFi.channel();
        command_digits.digit0 = action;

        int this_action_multiplier;

        switch (action)
        {
          case attack:
            this_action_multiplier = min(Multiplier, MAX_ATTACK_MULTIPLIER);
            break;

          case collect:
            this_action_multiplier = min(Multiplier, MAX_COLLECT_MULTIPLIER);
            break;

          case heal:
            this_action_multiplier = HEAL_MULTIPLIER; // player heals through pushing down of joystick.
            break;

          case poisonID:
            command_digits.digit0 = attack;
            this_action_multiplier = PARTICIPANT_MaxHP;
            break;

          default:
            this_action_multiplier = 0;
            break;
        }

        command_digits.digit1 = this_action_multiplier;

        ir_signal send_signal;
        send_signal.address = address_digits;
        send_signal.command = command_digits;

        Serial.printf("SEND %d %d %d %d | %d %d %d %d \n", address_digits.digit3, address_digits.digit2, address_digits.digit1, address_digits.digit0, \
                      command_digits.digit3, command_digits.digit2, command_digits.digit1, command_digits.digit0);
        Player_IR.send(send_signal, 1);

        start_receiving_feedback = millis();

        if ((EEPROM.read(isGL_add)) && (action == heal)) {
          HP--;
          EEPROM.write(PLAYER_HP_add, HP);
        }

        En--;
        EEPROM.write(PLAYER_EN_add, En);
        Serial.print("CURRENT MAC ADDRESS: ");
        Serial.println(WiFi.macAddress());
      }
    };

    void receiveAction() {
      int CLAN_, ID_, En_, MULTIPLIER_, action_, channel_; // underscore denotes details of IR signal sender
      unsigned long currTime = millis();
      if (Player_IR.available()) {
        ir_signal IRsignal_ = Player_IR.read();
        Serial.printf("SIG RECV ON %d %d %d\n", currTime, lastActionReceived, ACTION_RECV_WAIT);
        if (currTime - lastActionReceived > ACTION_RECV_WAIT) {
          CLAN_ = IRsignal_.address.digit2;
          ID_ = IRsignal_.address.digit0 + (IRsignal_.address.digit1 << 4);

          channel_ = IRsignal_.command.digit2;
          MULTIPLIER_ = IRsignal_.command.digit1;
          action_ = IRsignal_.command.digit0;


          Serial.printf("RECV %d %d %d %d | %d %d %d %d \n", IRsignal_.address.digit3, IRsignal_.address.digit2, IRsignal_.address.digit1, IRsignal_.address.digit0, IRsignal_.command.digit3, IRsignal_.command.digit2, IRsignal_.command.digit1, IRsignal_.command.digit0);

          lastActionReceived = currTime;

          if (((CLAN_ != CLAN) && (action_ == attack)) || ((action_ == heal) && (CLAN_ == CLAN) && (ID_ != ID)) || ((action_ == heal) && (CLAN_ != CLAN)) || (action_ == revive))
            handleAction(CLAN_, ID_, action_, MULTIPLIER_, channel_);
        }
      }
    }

    void update_player_state() {
      if (HP == 0) {
        En = 0;
        EEPROM.write(PLAYER_EN_add, En);
        permNoti = "    You Are Killed!     ";
        unsigned long currTime = millis();
        if (Multiplier > 1) {
          Multiplier = 1;
          EEPROM.write(PLAYER_MULTIPLIER_add, Multiplier);
        }
        poisonActive = false;
      }
      else {
        permNoti = "";
        unsigned long currTime = millis();
        last_max_en_decay = currTime;
        if (En >= MaxEn) {
          last_en_recover = currTime;
        }
        else {
          int en_rcv = EN_RECOVER_DURATION;
          if (is_x2EnRegen) {
            if (currTime - start_x2_en_regen < x2_En_Regen_bonus_duration) {
              en_rcv /= 2;
            }
            else {
              is_x2EnRegen = false;
            }
          }
          if (currTime - last_en_recover >= en_rcv) {
            En++ ;
            EEPROM.write(PLAYER_EN_add, En);
            last_en_recover = currTime;
          }
        }
      }
      EEPROM.commit();
    }

    void sync_state() {
      ID = EEPROM.read(ID_add);
      _isGL = EEPROM.read(isGL_add);
    }

    void handleJoystick_waiting() {
      joystick_pos joystick_pos = Player_joystick.read_Joystick();
      if (Player_joystick.get_state() == 0) {
        switch (joystick_pos)
        {
          case button:
            currentProcess = MainMenuProcess;
            Player_joystick.set_state();
            break;

          case idle:
            break;

          default:
            Player_joystick.set_state();
            break;
        }
      }
    }

    void handleJoystickInGame() {
      joystick_pos joystick_pos = Player_joystick.read_Joystick();
      if (Player_joystick.get_state() == 0) {
        switch (joystick_pos)
        {
          case up:
            if (poisonActive) {
              action = poisonID;
            } else {
              action = attack;
            }
            Player_joystick.set_state();
            break;

          case down:
            if (!EEPROM.read(isGL_add))
              action = collect;
            else
              action = heal;
            Player_joystick.set_state();
            break;

          case left:
            if (!choosingPowerUp) {
              lastPageNav--;
              if (lastPageNav > 4) lastPageNav -= 5;
              if (lastPageNav < 0) lastPageNav += 5;
              Nav_start = millis();
            }
            else {
              PowerUpNav-- ;
              if (PowerUpNav > 6) PowerUpNav -= 7;
              if (PowerUpNav < 0) PowerUpNav += 7;
            }
            Player_joystick.set_state();
            break;

          case right:
            if (!choosingPowerUp) {
              lastPageNav++;
              if (lastPageNav > 4) lastPageNav -= 5;
              if (lastPageNav < 0) lastPageNav += 5;
              Nav_start = millis();
            }
            else {
              PowerUpNav++ ;
              if (PowerUpNav > 6) PowerUpNav -= 7;
              if (PowerUpNav < 0) PowerUpNav += 7;
            }
            Player_joystick.set_state();
            break;

          case button:
            if (!choosingPowerUp) {
              if (lastPageNav != currentPage)
                currentPage = lastPageNav;
              if (currentPage == exitPage) {
                currentProcess = MainMenuProcess;
                currentPage = mainPage; // reset current page
                lastPageNav = currentPage;
              }
              if (currentPage == powerupPage) {
                choosingPowerUp = true;
                lastPageNav = mainPage;
              }
            }
            else {
              if (PowerUpNav == 0) {
                currentPage = mainPage;
                choosingPowerUp = false;
              }
              else {
                handlePowerUp(PowerUpNav);
              }
            }

            Player_joystick.set_state();
            break;

          default:
            action = do_nothing;
            if (lastPageNav != currentPage) {
              unsigned long currTime = millis();
              if (currTime - Nav_start > NAV_WAIT) lastPageNav = currentPage;
            }
            break;
        }
      }
      else {
        action = do_nothing;
        if (lastPageNav != currentPage) {
          unsigned long currTime = millis();
          if (currTime - Nav_start > NAV_WAIT) lastPageNav = currentPage;
        }
      }
    }

    void handlePowerUp(int powerup) {
      if (HP > 0) {
        switch (powerup)
        {
          case bonus6HP:
            if (num_bonus6HP > 0) {
              num_bonus6HP -- ;
              HP = min(HP + 6, MaxHP);
              EEPROM.write(PLAYER_num_bonus6HP_add, num_bonus6HP);
              EEPROM.write(PLAYER_HP_add, HP);
            }
            break;

          case bonus1MaxEn:
            if (num_bonus1MaxEn > 0) {
              num_bonus1MaxEn -- ;
              MaxEn++;
              EEPROM.write(PLAYER_num_bonus1MaxEn_add, num_bonus1MaxEn);
              EEPROM.write(PLAYER_MaxEn_add, MaxEn);
            }
            break;

          case bonus1MULTIPLIER:
            if (num_bonus1Multiplier > 0) {
              num_bonus1Multiplier -- ;
              Multiplier++;
              EEPROM.write(PLAYER_num_bonus1MULTIPLIER_add, num_bonus1Multiplier);
              EEPROM.write(PLAYER_MULTIPLIER_add, Multiplier);
            }
            break;

          case fiveminx2EnRegen:
            if (num_fiveminx2EnRegen > 0) {
              num_fiveminx2EnRegen -- ;
              start_x2_en_regen = millis();
              is_x2EnRegen = true;
              EEPROM.write(PLAYER_num_fiveminx2EnRegen_add, num_fiveminx2EnRegen);
            }
            break;

          case bomb:
            if (num_bomb > 0) {
              num_bomb -- ;
              Player_EspNOW.ScanForBombTarget();
              Player_EspNOW.SendBombToAllTargets(CLAN, ID);
              EEPROM.write(PLAYER_num_bomb_add, num_bomb);
              temp_bomb_attacked = 0;
              temp_bomb_killed = 0;
              active_bomb = true;
            }
            break;

          case poison:
            if (num_poison > 0) {
              num_poison -- ;
              poisonActive = true;
              EEPROM.write(PLAYER_num_poison_add, num_poison);
            }
            break;

          default:
            break;
        }
        PowerUpNav = 0;
      }
    }

    void handleAction(int CLAN_, int ID_, int action_, int MULTIPLIER_, int channel_) {
      Serial.print("Action ID: "); Serial.println(action_);
      switch (action_)
      {
        case attack:
          if (HP > 0) {
            HP = max(HP - MULTIPLIER_, 0);
            EEPROM.write(PLAYER_HP_add, HP);
            Serial.printf("Attacked. Current HP: %d \n", HP);
            tempNoti = "       Attacked      ";
            tempNoti_start = millis();
            feedback_attack(CLAN_, ID_, channel_);
            Player_Buzzer.sound(NOTE_E3);
          }
          break;

        case heal:
          if (HP > 0) {
            HP = min(HP + MULTIPLIER_, MaxHP);
            EEPROM.write(PLAYER_HP_add, HP);
            tempNoti = "        Healed       ";
            tempNoti_start = millis();
            last_received_heal = tempNoti_start;
          }
          break;


        case revive:
          if (HP == 0) {
            HP = HP + MULTIPLIER_;
            EEPROM.write(PLAYER_HP_add, HP);
            tempNoti = "        Revived       ";
            tempNoti_start = millis();
            last_received_heal = tempNoti_start;
          }
          break;

        case poisonID:
          if (HP > 0) {
            HP = max(HP - PARTICIPANT_MaxHP, 0);
            EEPROM.write(PLAYER_HP_add, HP);
            Serial.printf("Attacked. Current HP: %d \n", HP);
            tempNoti = "       Poisoned      ";
            tempNoti_start = millis();
            feedback_attack(CLAN_, ID_, channel_);
            Player_Buzzer.sound(NOTE_E3);
          }
          break;

        default:
          break;
      }
    }

    void feedback_attack(int CLAN_, int ID_, int channel_) {
      bool killed = (HP == 0);
      Player_EspNOW.send_data(1, 1, CLAN_, ID_, CLAN, killed, channel_);
    } ;

    void feedback_bomb(int CLAN_, int ID_) {
      bool killed = (HP == 0);
      Player_EspNOW.send_data(1, 4, CLAN_, ID_, CLAN, killed, 1); // TODO: Specify WiFi Channel
    }

    void handleFeedbackMsg(feedback_message feedbackData) {
      switch (feedbackData.attackee_type)
      {
        case 1:
          if ((feedbackData.attacker_CLAN == CLAN) && (feedbackData.attacker_ID == ID)) {
            if (feedbackData.is_attackee_killed == true) {
              tempNoti = " You killed a person ";
              tempNoti_start = millis();
              Multiplier++;
              numKilled ++ ;
              EEPROM.write(PLAYER_MULTIPLIER_add, Multiplier);
              EEPROM.write(PLAYER_numKilled_add, numKilled);
            }
            else {
              tempNoti = " Attack successfully ";
              tempNoti_start = millis();
            }
          }
          break;

        case 3:
          if ((feedbackData.attacker_CLAN == CLAN) && (feedbackData.attacker_ID == ID)) {
            if (feedbackData.is_attackee_killed == true) {
              tempNoti = " U Opened L2 Treasure";
              tempNoti_start = millis();
              numL2Treasure ++ ;
              if (feedbackData.powerup_received == 6) {
                num_poison ++ ;
                EEPROM.write(PLAYER_num_poison_add, num_poison);
                tempNoti = " PowerUp: A Poison!!  ";
              }

              break;

            }
            else {
              tempNoti = " L2 Treasure Damaged ";
              tempNoti_start = millis();
            }
          }
          break;

        case 2:
          if ((feedbackData.attacker_CLAN == CLAN) && (feedbackData.attacker_ID == ID)) {
            Serial.print("L1 Treasure Collected Power Up:"); Serial.println(feedbackData.powerup_received);
            numL1Treasure++;
            switch (feedbackData.powerup_received)
            {
              case bonus6HP:
                num_bonus6HP ++ ;
                EEPROM.write(PLAYER_num_bonus6HP_add, num_bonus6HP);
                tempNoti = "    PowerUp: +6 HP   ";
                break;

              case bonus1MaxEn:
                num_bonus1MaxEn ++ ;
                EEPROM.write(PLAYER_num_bonus1MaxEn_add, num_bonus1MaxEn);
                tempNoti = "  PowerUp: +1 Max En ";
                break;

              case bonus1MULTIPLIER:
                num_bonus1Multiplier ++ ;
                EEPROM.write(PLAYER_num_bonus1MULTIPLIER_add, num_bonus1Multiplier);
                tempNoti = "   PowerUp: +1 MULTIPLIER  ";
                break;

              case fiveminx2EnRegen:
                num_fiveminx2EnRegen ++ ;
                EEPROM.write(PLAYER_num_fiveminx2EnRegen_add, num_fiveminx2EnRegen);
                tempNoti = "PowerUp: x2 En Regen ";
                break;

              case bomb:
                num_bomb ++ ;
                EEPROM.write(PLAYER_num_bomb_add, num_bomb);
                tempNoti = "  PowerUp: A Bomb!!  ";
                break;

              default:
                break;
            }
            tempNoti_start = millis();
          }
          break;

        case 4:
          if ((feedbackData.attacker_CLAN == CLAN) && (feedbackData.attacker_ID == ID)) {
            if (feedbackData.powerup_received == true) {
              temp_bomb_killed += 1;
              temp_bomb_attacked += 1;
              Multiplier++;
              numKilled ++ ;
              EEPROM.write(PLAYER_MULTIPLIER_add, Multiplier);
              EEPROM.write(PLAYER_numKilled_add, numKilled);
            }
            else {
              temp_bomb_attacked += 1;
            }
          }
          break;

        case 5:
          // received a heal feedback
          if (HP != 0) En++;
          HP = MaxHP;
          EEPROM.write(PLAYER_HP_add, HP);
          tempNoti = "        Healed       ";
          tempNoti_start = millis();
          last_received_heal = tempNoti_start;
          break;


        default:
          break;
      }
    }

    void handleBombed(feedback_message feedbackData) {
      if (HP > 0) {
        HP = std::max(HP - BOMB_HP_DEDUCTION, 0);
        std::tie(tempNoti, tempNoti_start) = std::make_pair("   You are Bombed!!  ", millis());
        Player_Buzzer.sound(NOTE_E3);
        feedback_bomb(feedbackData.attacker_CLAN, feedbackData.attacker_ID);
      }
    }

    void receiveEspNOW() {
      if (EspNOW_received >= 1) {
        int i;
        for (i = 0; i < EspNOW_received; i++) {
          feedback_message feedbackData = Player_EspNOW.get_feedback_received();
          switch (feedbackData.msg_type)
          {
            case 1:
              handleFeedbackMsg(feedbackData);
              break;

            case 2:
              handleBombed(feedbackData);
              break;

            default:
              break;
          }
          EspNOW_received -- ;
        }
        if ((temp_bomb_attacked == Player_EspNOW.num_bombed) && (active_bomb)) {
          Serial.print("Bombed "); Serial.println(temp_bomb_attacked);
          String num_attack_noti = String(" Bombed ");
          num_attack_noti.concat(temp_bomb_attacked);
          String num_killed_noti = String(" Killed ");
          num_killed_noti.concat(temp_bomb_killed);
          num_attack_noti += num_killed_noti;
          tempNoti = num_attack_noti;
          tempNoti_start = millis();
          temp_bomb_attacked = 0;
          temp_bomb_killed = 0;
          active_bomb = false;
        }
      }
    } ;

    void update_display_waiting() {
      TreasureHunt_OLED.display_WaitingPage();
    }

    void update_display() {
      String noti_to_display;
      if (permNoti.length() == 0) {
        if (tempNoti.length() != 0) {
          unsigned long currTime = millis();
          if (currTime - tempNoti_start <= TEMP_NOTI_WAIT) {
            if (((currTime - tempNoti_start) / TEMP_NOTI_BLINKING) % 2 == 0)
              noti_to_display = tempNoti;
            else
              noti_to_display = " ";
          }
          else {
            noti_to_display = "";
            tempNoti = "";
          }
        }
      }
      else {
        noti_to_display = permNoti;
      }

      switch (currentPage)
      {
        case mainPage:
          TreasureHunt_OLED.display_mainPage(HP, En, noti_to_display, lastPageNav);
          break;
        case infoPage:
          TreasureHunt_OLED.display_infoPage(CLAN, ID, Multiplier, MaxEn, noti_to_display, lastPageNav);
          break;
        case achievementPage:
          TreasureHunt_OLED.display_achievementPage(numKilled, numL1Treasure, numL2Treasure, noti_to_display, lastPageNav);
          break;
        case powerupPage:
          TreasureHunt_OLED.display_powerupPage(num_bonus6HP,
                                                num_bonus1MaxEn,
                                                num_bonus1Multiplier,
                                                num_fiveminx2EnRegen,
                                                num_bomb,
                                                num_poison,
                                                noti_to_display,
                                                PowerUpNav);
          break;

        default:
          lastPageNav = mainPage;
          currentPage = mainPage;
      }
    }

    void update_sound() {
      unsigned long currTime = millis();
      if (currTime - tempNoti_start >= NOTI_SOUND_DURATION) {
        Player_Buzzer.end_sound();
      }
    };

    int get_game_state() {
      // retrieve game state from server
      // 0 mean game did not start
      // 1 mean in game
      // once the game has started then we dunnid to check anymore
      if (!gameStarted) {
        gameStarted = game_started_buffer;
        if (gameStarted) {
          Serial.println("Game has started! Starting initialisation processes...");
          int CLAN = EEPROM.read(CLAN_add);
          bool isGL = EEPROM.read(isGL_add);
          int id;

          if (WIFI_ON) {
            // first check if wanderer has been registered
            while (EEPROM.read(PLAYER_registered_online_add) != 1) {
              Serial.println("Attempting to register WANDERER...");
              EEPROM.write(PLAYER_registered_online_add, dbc.registerWanderer(CLAN, my_MAC_address));
            }

            String id_constants_json = dbc.getPlayerIDandGameConstantsJSON(CLAN, my_MAC_address);
            id = dbc.getPlayerIDFromJSON(id_constants_json);
            EEPROM.write(ID_add, id);

            GAME_CONSTANTS game_consts = dbc.retrieveGameConstantsFromJSONArray(id_constants_json);
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
          } else {
            Serial.println("WIFI MODE DISABLED, retrieving ID and constants from hardcoded memory");
            id = EEPROM.read(ID_add);
          }

          Serial.printf("[INITIALISE] Current CLAN: %d ID %d\n", CLAN, EEPROM.read(ID_add));
          setup_initial_state(id, CLAN, isGL); // initialize Player
          deviceReady = 1;
        }
        return gameStarted;
      } else return 1;
    }

    void gameMainLoop() {
      if (!get_game_state()) {
        handleJoystick_waiting();
        update_display_waiting();
      }
      else {
        sync_state();  // sync internal variables with EEPROM state
        handleJoystickInGame();
        sendAction();
        receiveAction();
        receiveEspNOW();
        update_player_state();
        update_display();
        update_sound();
        EEPROM.commit();
      }
    };

    void gameBackgroundProcess() {
      if (deviceReady == 1) {
        unsigned long currTime = millis();
        /*
           Considering 250 devices on at the same time, and each request takes 2-3s for roundtrip.
           If we were to stagger the upload periods of all devices, that will take 250 * 3 = 750s === 12 mins
           We can accept a small room of overlap, hence we set a upload period of 10 mins.
        */
        if (currTime - last_update_kills_to_server > KILL_UPDATE_SERVER_INTERVAL) {
          // First make sure player has been registered on server
          dbc.connectToWiFi();
          if (EEPROM.read(PLAYER_registered_online_add) != 1) {
            // register first
            EEPROM.write(PLAYER_registered_online_add, dbc.registerWanderer(CLAN, my_MAC_address));
          }
          Serial.println("Sending Game Statistics to Server...");
          FailedFeedbackStatistics this_stats;
          this_stats = dbc.sendGameStatistics(CLAN, ID, numKilled, numL1Treasure, numL2Treasure);
          int unrecognized_kills = this_stats.num_kills;
          int unrecognized_powerups = this_stats.num_powerups;
          Multiplier += unrecognized_kills ;
          EEPROM.write(PLAYER_MULTIPLIER_add, Multiplier);
          tempNoti = "   Kills Retrieved   ";
          tempNoti_start = millis();
          numL1Treasure += unrecognized_powerups ;
          EEPROM.write(PLAYER_numL1Treasure_add, numL1Treasure);
          int i;
          for (i = 0; i < unrecognized_powerups; i ++) {
            int new_PowerUp = random(1, 6);
            switch (new_PowerUp)
            {
              case bonus6HP:
                num_bonus6HP ++ ;
                EEPROM.write(PLAYER_num_bonus6HP_add, num_bonus6HP);
                break;

              case bonus1MaxEn:
                num_bonus1MaxEn ++ ;
                EEPROM.write(PLAYER_num_bonus1MaxEn_add, num_bonus1MaxEn);
                break;

              case bonus1MULTIPLIER:
                num_bonus1Multiplier ++ ;
                EEPROM.write(PLAYER_num_bonus1MULTIPLIER_add, num_bonus1Multiplier);
                break;

              case fiveminx2EnRegen:
                num_fiveminx2EnRegen ++ ;
                EEPROM.write(PLAYER_num_fiveminx2EnRegen_add, num_fiveminx2EnRegen);
                break;

              case bomb:
                num_bomb ++ ;
                EEPROM.write(PLAYER_num_bomb_add, num_bomb);
                break;

              default:
                break;
            }
            tempNoti = " Power-Ups Retrieved ";
            tempNoti_start = millis();
          }
          last_update_kills_to_server = currTime;
          EEPROM.commit();
          if (failed_kill_feedback > 0) {
            int i;
            for (i = 0; i < failed_kill_feedback; i++) {
              int this_ID = failed_kill_ID[current_failed_read_pointer] ;
              int this_CLAN = failed_kill_CLAN[current_failed_read_pointer] ;
              dbc.uploadFailedFeedback(this_CLAN, this_ID);
              current_failed_read_pointer ++ ;
              if (current_failed_read_pointer > 50) current_failed_read_pointer -= 50 ;
              failed_kill_feedback --;
              Serial.println("Upload Done");
            }
          } else delay(50);

          WiFi.disconnect();
          dbc.changeWiFiChannel(dbc.getClanWiFiChannel(CLAN));
        } else {
          delay(50);
        }
      }
      else {
        delay(50);
      }
    }
};

TreasureHuntPlayer PLAYER;
