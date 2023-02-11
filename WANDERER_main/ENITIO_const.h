int currentProcess = 0;
String my_MAC_address;
uint8_t my_MAC_address_arr[6] = {};

const int MainMenuProcess = 0 ;
const int ProfileProcess = 1 ;
const int TreasureHuntProcess = 2 ; 
const int SnakeGameProcess = 3 ;
const int MusicProcess = 4 ; 
const int AdminProcess = 5 ;
const int CreditProcess = 6 ;

#define NAV_WAIT 4000

#define EEPROM_SIZE 20

const int PROFILE_enable_add = 0;
const int CLAN_add = 1;
const int NAME_add = 2;
const int isGL_add = 3;
const int ID_add = 4;
const int PLAYER_enable_add = 5;
const int PLAYER_HP_add = 6;
const int PLAYER_EN_add = 7;
const int PLAYER_MaxHP_add = 8;
const int PLAYER_MaxEn_add = 9;
const int PLAYER_MULTIPLIER_add = 10;
const int PLAYER_numKilled_add = 11;
const int PLAYER_numL1Treasure_add = 12;
const int PLAYER_numL2Treasure_add = 13;
const int PLAYER_num_bonus6HP_add = 14;
const int PLAYER_num_bonus1MaxEn_add = 15;
const int PLAYER_num_bonus1MULTIPLIER_add = 16;
const int PLAYER_num_fiveminx2EnRegen_add = 17;
const int PLAYER_num_bomb_add = 18;


int EN_RECOVER_DURATION;
int VIRUS_DECAY_DURATION;
int VIRUS_IMMUNITY_DURATION;
int VIRUS_INFECTION_PROBABILITY;  // integer between 0 and 100
int PARTICIPANT_MaxHP;
int GL_MaxHP;
int PARTICIPANT_MaxEn;
int GL_MaxEn;
int INITIAL_MULTIPLIER;
int MAX_ATTACK_MULTIPLIER;
int MAX_COLLECT_MULTIPLIER;
int BOMB_HP_DEDUCTION;
int KILL_UPDATE_SERVER_INTERVAL;
int revival_MULTIPLIER;


int failed_kill_feedback = 0;
int failed_kill_CLAN[50] = {};
int failed_kill_ID[50] = {};
int current_failed_save_pointer = 0;
int current_failed_read_pointer = 0;


