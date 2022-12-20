/**************************************************************************
    Stars2021 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
*/

#include "BSOS_Config.h"
#include "BallySternOS.h"
#include "AudioHandler.h"
#include "DropTargets.h"
#include "SelfTestAndAudit.h"
#include "BigGame.h"
#include "BigGamePrey.h"
#include "LampAnimations.h"
#include "BigGameIntro.h"
#include <EEPROM.h>

/***********************
   Bugs & features
    --option for how many cards can be lit
    --Attract mode
    --rally animation
    Need to update sound card
    payout of bonus letters
    --No award for standups with lamp flashing
    --wrong left spinner value
    right spinner lamp & increased value (number 53)
    right lane with Z
    --left spinner with X
    Reserve bonus payout
    --Payout of BIGGAME level is long and weird
    --sound of BIGGAME letters isn't good
    --better sound for BIGGAME
    --multi players digit shift

    --Drop target bonus lamps not on & paying out
    --animation for all cards filled (radar)
    animation for big game filled
    --hurry up jackpot after big game filled (saucer)
    voice prompt cut off for multi-players at start of rally
    --match digits wrong (2 & 3 instead of 1 & 2)

    --EB & special once per game
    --Big game sound at start of payout/no payout
    --no re-big game until another switch hit (any)
    --clear rain at end of rain cycle in intro
    --big lights when saucer lands
    --grace period?
    --reset button during match sequence
    --one card at a time after first clear?
    --during intro, extra 5k light is on
    

    rollovers:
      SW_TOP_RIGHT_ROLLOVER_BUTTON
      SW_MID_RIGHT_ROLLOVER_BUTTON
      SW_LOWER_RIGHT_ROLLOVER_BUTTON

    Settings:
      big game jackpot time - off, 10, 20, 30 ,40, 50, 60 seconds
      hurry up drops - never, first card, first two cards, always
      award all cards for hurry up (liberal clear rule)
      max cards lit - one, two, three
      max cards lit decreases with level
      credit/reset button - no reset, immediate, 1 second, 2 seconds, 3 seconds

*/

#define BIG_GAME_2022_MAJOR_VERSION  2022
#define BIG_GAME_2022_MINOR_VERSION  1
#define DEBUG_MESSAGES  1

/*********************************************************************

    Game specific code

*********************************************************************/

// MachineState
//  0 - Attract Mode
//  negative - self-test modes
//  positive - game play
char MachineState = 0;
boolean MachineStateChanged = true;
#define MACHINE_STATE_ATTRACT         0
#define MACHINE_STATE_INIT_GAMEPLAY   1
#define MACHINE_STATE_INIT_NEW_BALL   2
#define MACHINE_STATE_NORMAL_GAMEPLAY 4
#define MACHINE_STATE_COUNTDOWN_BONUS 99
#define MACHINE_STATE_BALL_OVER       100
#define MACHINE_STATE_MATCH_MODE      110


#define MACHINE_STATE_ADJUST_FREEPLAY           (MACHINE_STATE_TEST_DONE-1)
#define MACHINE_STATE_ADJUST_BALL_SAVE          (MACHINE_STATE_TEST_DONE-2)
#define MACHINE_STATE_ADJUST_SFX_AND_SOUNDTRACK (MACHINE_STATE_TEST_DONE-3)
#define MACHINE_STATE_ADJUST_MUSIC_VOLUME       (MACHINE_STATE_TEST_DONE-4)
#define MACHINE_STATE_ADJUST_SFX_VOLUME         (MACHINE_STATE_TEST_DONE-5)
#define MACHINE_STATE_ADJUST_CALLOUTS_VOLUME    (MACHINE_STATE_TEST_DONE-6)
#define MACHINE_STATE_ADJUST_BIG_GAME_JACKPOT_TIME    (MACHINE_STATE_TEST_DONE-7)
#define MACHINE_STATE_ADJUST_HURRY_UP_DROP_LEVEL      (MACHINE_STATE_TEST_DONE-8)
#define MACHINE_STATE_ADJUST_HURRY_UP_DROPS_LIBERAL   (MACHINE_STATE_TEST_DONE-9)
#define MACHINE_STATE_ADJUST_MAX_CARDS_LIT            (MACHINE_STATE_TEST_DONE-10)
#define MACHINE_STATE_ADJUST_MAX_CARDS_LIT_DECREASES  (MACHINE_STATE_TEST_DONE-11)
#define MACHINE_STATE_ADJUST_CREDIT_RESET_HOLD_TIME   (MACHINE_STATE_TEST_DONE-12)
#define MACHINE_STATE_ADJUST_TOURNAMENT_SCORING (MACHINE_STATE_TEST_DONE-13)
#define MACHINE_STATE_ADJUST_TILT_WARNING       (MACHINE_STATE_TEST_DONE-14)
#define MACHINE_STATE_ADJUST_AWARD_OVERRIDE     (MACHINE_STATE_TEST_DONE-15)
#define MACHINE_STATE_ADJUST_BALLS_OVERRIDE     (MACHINE_STATE_TEST_DONE-16)
#define MACHINE_STATE_ADJUST_SCROLLING_SCORES   (MACHINE_STATE_TEST_DONE-17)
#define MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD   (MACHINE_STATE_TEST_DONE-18)
#define MACHINE_STATE_ADJUST_SPECIAL_AWARD      (MACHINE_STATE_TEST_DONE-19)
//#define MACHINE_STATE_ADJUST_DIM_LEVEL          (MACHINE_STATE_TEST_DONE-21)
#define MACHINE_STATE_ADJUST_DONE               (MACHINE_STATE_TEST_DONE-20)

byte SelfTestStateToCalloutMap[] = {
  136, 137, 135, 134, 133, 140, 141, 142, 139, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158,
  200, 201, 206, 207, 208, 209, 
  159, 160, 161, 162, 163, 164, 165, 0
};

#define SOUND_EFFECT_AP_HUD_OPTION_1    202
#define SOUND_EFFECT_AP_HUD_OPTION_99   205
#define SOUND_EFFECT_AP_CRB_OPTION_1    211
#define SOUND_EFFECT_AP_CRB_OPTION_99   210

// The lower 4 bits of the Game Mode are modes, the upper 4 are for frenzies
// and other flags that carry through different modes
#define GAME_MODE_SKILL_SHOT_PRE_SWITCH             0
#define GAME_MODE_SKILL_SHOT_BUILD_BONUS            1
#define GAME_MODE_UNSTRUCTURED_PLAY                 2
#define GAME_MODE_ALL_PREY_KILLED                   3
#define GAME_MODE_BIG_GAME_SAUCER_START             4
#define GAME_MODE_BIG_GAME_SAUCER_NO_PAYOUT         5
#define GAME_MODE_BIG_GAME_SAUCER_PAYOUT            6
#define GAME_MODE_BIG_GAME_JACKPOT_COUNTDOWN        7
#define GAME_MODE_BIG_GAME_JACKPOT                  8


#define EEPROM_BALL_SAVE_BYTE           100
#define EEPROM_FREE_PLAY_BYTE           101
#define EEPROM_SOUND_SELECTOR_BYTE      102
#define EEPROM_SKILL_SHOT_BYTE          103
#define EEPROM_TILT_WARNING_BYTE        104
#define EEPROM_AWARD_OVERRIDE_BYTE      105
#define EEPROM_BALLS_OVERRIDE_BYTE      106
#define EEPROM_TOURNAMENT_SCORING_BYTE  107
#define EEPROM_MUSIC_VOLUME_BYTE        108
#define EEPROM_SFX_VOLUME_BYTE          109
#define EEPROM_SCROLLING_SCORES_BYTE    110
#define EEPROM_CALLOUTS_VOLUME_BYTE     111
#define EEPROM_DIM_LEVEL_BYTE           113
#define EEPROM_BG_JACKPOT_TIME          114
#define EEPROM_HUD_LEVEL                115
#define EEPROM_HUD_LIBERAL              116
#define EEPROM_MAX_CARDS_LIT            117
#define EEPROM_MAX_CARDS_DECREASES      118
#define EEPROM_CRB_HOLD_TIME            119 
#define EEPROM_EXTRA_BALL_SCORE_BYTE    140
#define EEPROM_SPECIAL_SCORE_BYTE       144

#define SOUND_EFFECT_NONE                     0
#define SOUND_EFFECT_TILT                     1
#define SOUND_EFFECT_TILT_WARNING             2
#define SOUND_EFFECT_INLANE_UNLIT             3
#define SOUND_EFFECT_OUTLANE_UNLIT            4
#define SOUND_EFFECT_INLANE_LIT               5
#define SOUND_EFFECT_OUTLANE_LIT              6
#define SOUND_EFFECT_SKILL_SHOT               8
#define SOUND_EFFECT_MATCH_SPIN               10
#define SOUND_EFFECT_DROP_TARGET_HIT          11
#define SOUND_EFFECT_DROP_TARGET_FINISHED     12
#define SOUND_EFFECT_DROP_TARGET_HURRY_UP     13
#define SOUND_EFFECT_DROP_TARGET_HURRY_UP_1   14
#define SOUND_EFFECT_TOP_LANE                 15
#define SOUND_EFFECT_BIG_GAME_LEVEL_COLLECT   16
#define SOUND_EFFECT_JACKPOT                  19
#define SOUND_EFFECT_EXPLOSION                20
#define SOUND_EFFECT_CARD_FINISH              21
#define SOUND_EFFECT_WATER_DROP_1             22
#define SOUND_EFFECT_WATER_DROP_2             22
#define SOUND_EFFECT_WATER_DROP_3             22
#define SOUND_EFFECT_LAUNCH                   25
#define SOUND_EFFECT_BUZZ_AWARD               26
#define SOUND_EFFECT_EXTRA_BALL               27
#define SOUND_EFFECT_SLING_SHOT               34
#define SOUND_EFFECT_ROLLOVER                 35
#define SOUND_EFFECT_ANIMATION_TICK           36
#define SOUND_EFFECT_BUMPER_HIT_STD_1         40
#define SOUND_EFFECT_BUMPER_HIT_STD_2         41
#define SOUND_EFFECT_BUMPER_HIT_STD_3         42
#define SOUND_EFFECT_BUMPER_HIT_STD_4         43
#define SOUND_EFFECT_BUMPER_HIT_STD_5         44
#define SOUND_EFFECT_BUMPER_HIT_STD_6         45
#define SOUND_EFFECT_BUMPER_HIT_STD_7         46
#define SOUND_EFFECT_BUMPER_HIT_STD_8         47
#define SOUND_EFFECT_GUN_COCK_1               50
#define SOUND_EFFECT_GUN_COCK_2               51
#define SOUND_EFFECT_GUN_COCK_3               52
#define SOUND_EFFECT_GUN_COCK_4               53
#define SOUND_EFFECT_GUN_COCK_5               54
#define SOUND_EFFECT_GUN_COCK_6               55
#define SOUND_EFFECT_GUN_COCK_7               56
#define SOUND_EFFECT_GUN_COCK_8               57
#define SOUND_EFFECT_REVOLVER_CLICK_1         58
#define SOUND_EFFECT_CLICK                    59
#define SOUND_EFFECT_LONG_ELEPHANT            62
#define SOUND_EFFECT_VERY_SHORT_ELEPHANT      63
#define SOUND_EFFECT_LEFT_SPINNER_STD         65
#define SOUND_EFFECT_RIGHT_SPINNER_STD        66
#define SOUND_EFFECT_MEDIUM_ELEPHANT          60
#define SOUND_EFFECT_LETTER_ROLLOVER          61
#define SOUND_EFFECT_SHORT_ELEPHANT           63
#define SOUND_EFFECT_JUNGLE_BIRD_CALL         69
#define SOUND_EFFECT_HEARTBEAT                70
#define SOUND_EFFECT_FOOTSTEP_1               71
#define SOUND_EFFECT_FOOTSTEP_2               72
#define SOUND_EFFECT_FOOTSTEP_3               73
#define SOUND_EFFECT_FOOTSTEP_4               74
#define SOUND_EFFECT_FOOTSTEP_5               75
#define SOUND_EFFECT_FOOTSTEP_6               76
#define SOUND_EFFECT_FOOTSTEP_7               77
#define SOUND_EFFECT_FOOTSTEP_8               78
#define SOUND_EFFECT_PANTING                  79
#define SOUND_EFFECT_COIN_DROP_1              100
#define SOUND_EFFECT_COIN_DROP_2              101
#define SOUND_EFFECT_COIN_DROP_3              102
#define SOUND_EFFECT_MACHINE_START            318
#define SOUND_EFFECT_BALL_OVER                319

//#define SOUND_EFFECT_SELF_TEST_MODE_START             133
#define SOUND_EFFECT_SELF_TEST_CPC_START              180
#define SOUND_EFFECT_SELF_TEST_AUDIO_OPTIONS_START    190


#define RALLY_MUSIC_WAITING_FOR_SKILLSHOT         0
#define RALLY_MUSIC_WAITING_FOR_SKILLSHOT_CALM    1
#define RALLY_MUSIC_WAITING_FOR_SKILLSHOT_UPBEAT  3
#define RALLY_MUSIC_WAITING_FOR_SKILLSHOT_MEDIUM  2

// General music
#define SOUND_EFFECT_BACKGROUND_SONG_1        700
#define SOUND_EFFECT_BACKGROUND_SONG_2        701
#define SOUND_EFFECT_BACKGROUND_SONG_3        702
#define SOUND_EFFECT_BACKGROUND_SONG_4        703
#define SOUND_EFFECT_BACKGROUND_SONG_18       717
#define SOUND_EFFECT_EXPECTANT_MUSIC          790

// For each music set, there are five types of music
#define MUSIC_TYPE_UNSTRUCTURED_PLAY  0
#define MUSIC_TYPE_SIDE_QUEST         1
#define MUSIC_TYPE_MISSION            2
#define MUSIC_TYPE_WIZARD             3
#define MUSIC_TYPE_RALLY              4
//

byte CurrentMusicType = 0xFF; // value can be 0-3

AudioSoundtrack SoundtrackUnstructured[10] = {
  //  {700, 10}, {701, 10}, {702, 10}, {703, 10}, {704, 10}, {705, 10}, {706, 10}, {707, 10}, {708, 10}, {709, 10}
  {700, 267}, {701, 237}, {702, 84}, {703, 127}, {704, 96}, {705, 203}, {706, 75}, {707, 107}, {708, 135}, {709, 294}
};

// Game play status callouts
#define NUM_VOICE_NOTIFICATIONS                 9
byte VoiceNotificationDurations[NUM_VOICE_NOTIFICATIONS] = {
  4, 4, 4, 4, 4, 4, 4, 4, 4
};

#define SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START  405

#define SOUND_EFFECT_VP_ADD_PLAYER_1        405
#define SOUND_EFFECT_VP_ADD_PLAYER_2        (SOUND_EFFECT_ADD_PLAYER_1+1)
#define SOUND_EFFECT_VP_ADD_PLAYER_3        (SOUND_EFFECT_ADD_PLAYER_1+2)
#define SOUND_EFFECT_VP_ADD_PLAYER_4        (SOUND_EFFECT_ADD_PLAYER_1+3)
#define SOUND_EFFECT_VP_PLAYER_1_UP         409
#define SOUND_EFFECT_VP_PLAYER_2_UP         (SOUND_EFFECT_PLAYER_1_UP+1)
#define SOUND_EFFECT_VP_PLAYER_3_UP         (SOUND_EFFECT_PLAYER_1_UP+2)
#define SOUND_EFFECT_VP_PLAYER_4_UP         (SOUND_EFFECT_PLAYER_1_UP+3)
#define SOUND_EFFECT_VP_SHOOT_AGAIN         413

#define MAX_DISPLAY_BONUS     120
#define TILT_WARNING_DEBOUNCE_TIME      1000


/*********************************************************************

    Machine state and options

*********************************************************************/
byte SoundtrackSelection = 0;
byte Credits = 0;
byte SoundSelector = 6;
byte BallSaveNumSeconds = 0;
byte CurrentBallSaveNumSeconds = 0;
byte MaximumCredits = 40;
byte BallsPerGame = 3;
byte DimLevel = 2;
byte ScoreAwardReplay = 0;
byte MusicVolume = 10;
byte SoundEffectsVolume = 10;
byte CalloutsVolume = 10;
byte ChuteCoinsInProgress[3];
byte TimeRequiredToResetGame = 1;
boolean HighScoreReplay = true;
boolean MatchFeature = true;
boolean TournamentScoring = false;
boolean ScrollingScores = true;
boolean FreePlayMode = false;
unsigned long HighScore = 0;
unsigned long AwardScores[3];
unsigned long ExtraBallValue = 0;
unsigned long SpecialValue = 0;
unsigned long CurrentTime = 0;
unsigned long SoundSettingTimeout = 0;
unsigned long CreditResetPressStarted = 0;

AudioHandler Audio;


/*********************************************************************

    Game State

*********************************************************************/
byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
byte BonusCountdownProgress;
byte BonusX;
byte GameMode = GAME_MODE_SKILL_SHOT_PRE_SWITCH;
byte MaxTiltWarnings = 2;
byte NumTiltWarnings = 0;
byte CurrentAchievements[4];

boolean SamePlayerShootsAgain = false;
boolean BallSaveUsed = false;
boolean LowerExtraBallCollected[4];
boolean LowerSpecialCollected[4];
boolean UpperExtraBallCollected[4];
boolean UpperSpecialCollected[4];
boolean ShowingModeStats = false;
boolean PlayScoreAnimationTick = true;
boolean Drops123Done = false;
boolean Drops456Done = false;
boolean Drops789Done = false;

unsigned long CurrentScores[4];
unsigned long BallFirstSwitchHitTime = 0;
unsigned long BallTimeInTrough = 0;
unsigned long GameModeStartTime = 0;
unsigned long GameModeEndTime = 0;
unsigned long LastTiltWarningTime = 0;
unsigned long ScoreAdditionAnimation;
unsigned long ScoreAdditionAnimationStartTime;
unsigned long LastRemainingAnimatedScoreShown;
unsigned long PlayfieldMultiplier;
unsigned long BallSaveEndTime;
unsigned long AnimateSkillShotStartTime;

#define BALL_SAVE_GRACE_PERIOD  1500


/*********************************************************************

    Game Specific State Variables

*********************************************************************/
byte DTBankHitOrder;
byte SpinnerHits[4];
byte CardSelected[4];
byte PreyLevel[4];
byte Pop1Hits;
byte Pop2Hits;
byte Pop3Hits;
byte BigGameStatus[4];
byte BigGameLevel[4];
byte BigGameAwarded[4];
byte CardLevel[4];
byte SkillShotLetter;
byte LastStatus;
byte RightSpinnerAnimationPhase = 0;
byte BonusLineBall = 2;
byte Drop123HurryUpPhase = 0;
byte Drop456HurryUpPhase = 0;
byte Drop789HurryUpPhase = 0;
byte BigGameLevelCollect;
byte BigGameLetterCollect;
byte BigGameCollectPhase;
byte HurryUpDropLevel = 1;
byte MaxCardsLit = 2;
byte SnakeRolloverStatus;
byte BigGameJackpotTime = 10;


boolean TimersPaused = true;
boolean DisplaysNeedResetting;
boolean HidePreyOnCards = false;
boolean LowerExtraBall = false;
boolean LowerSpecial = false;
boolean LiberalClearRule = true;
boolean SaucerPayout = false;
boolean SaucerPrimed = true;
boolean BigGamePayoutWithCreditButton = false;
boolean MaxCardsAffectedByLevel = true;

unsigned long LastSpinnerHit;
unsigned long BonusXAnimationStart;
unsigned long LastSwitchHitTime;
unsigned long LastSaucerHitTime;
unsigned long LastModePrompt;
unsigned long LastAlternatingScore;
unsigned long SkillShotScore;
unsigned long BigGameJackpotEndTime;

unsigned long RevealCardsStartTime;
unsigned long CardLevelIncreaseTime;
unsigned long BIGGAMELevelIncreaseTime;

unsigned long CardStatus[4];
unsigned long CardScan;
unsigned long CardWipe;
unsigned long GameLampsHitTime[4];

unsigned long LastTimeThroughLoop = 0;
unsigned long TimerTicks = 0;
unsigned long LastTimeStatusUpdated = 0;
unsigned long RightSpinnerAnimationStart = 0;
unsigned long Drop123HurryUp;
unsigned long Drop456HurryUp;
unsigned long Drop789HurryUp;
unsigned long BIGHurryUpStart;
unsigned long BigGameJackpotStart;
unsigned long SnakeRolloverStart;
unsigned long SaucerSwitchStartTime;


DropTargetBank DropTargets123(3, 1, DROP_TARGET_TYPE_STERN_1, 12);
DropTargetBank DropTargets456(3, 1, DROP_TARGET_TYPE_STERN_1, 12);
DropTargetBank DropTargets789(3, 1, DROP_TARGET_TYPE_STERN_1, 12);

#define CARD_X_SELECTED 1
#define CARD_Y_SELECTED 2
#define CARD_Z_SELECTED 4

#define PREY_KILLED_REWARD  20000
#define BIG_HURRY_UP_DURATION 10000

#define BIG_GAME_LETTER_B   0x40
#define BIG_GAME_LETTER_I   0x20
#define BIG_GAME_LETTER_G1  0x10
#define BIG_GAME_LETTER_G2  0x08
#define BIG_GAME_LETTER_A   0x04
#define BIG_GAME_LETTER_M   0x02
#define BIG_GAME_LETTER_E   0x01

BigGamePrey Prey[4];

void ReadStoredParameters() {
  for (byte count = 0; count < 3; count++) {
    ChuteCoinsInProgress[count] = 0;
  }

  HighScore = BSOS_ReadULFromEEProm(BSOS_HIGHSCORE_EEPROM_START_BYTE, 10000);
  Credits = BSOS_ReadByteFromEEProm(BSOS_CREDITS_EEPROM_BYTE);
  if (Credits > MaximumCredits) Credits = MaximumCredits;

  ReadSetting(EEPROM_FREE_PLAY_BYTE, 0);
  FreePlayMode = (EEPROM.read(EEPROM_FREE_PLAY_BYTE)) ? true : false;

  BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 15);
  if (BallSaveNumSeconds > 20) BallSaveNumSeconds = 20;
  CurrentBallSaveNumSeconds = BallSaveNumSeconds;

  SoundSelector = ReadSetting(EEPROM_SOUND_SELECTOR_BYTE, 6);
  if (SoundSelector!=0 || SoundSelector>9) SoundSelector = 6;
  SoundtrackSelection = 0;

  MusicVolume = ReadSetting(EEPROM_MUSIC_VOLUME_BYTE, 10);
  if (MusicVolume == 0 || MusicVolume > 10) MusicVolume = 10;

  SoundEffectsVolume = ReadSetting(EEPROM_SFX_VOLUME_BYTE, 10);
  if (SoundEffectsVolume == 0 || SoundEffectsVolume > 10) SoundEffectsVolume = 10;

  CalloutsVolume = ReadSetting(EEPROM_CALLOUTS_VOLUME_BYTE, 10);
  if (CalloutsVolume == 0 || CalloutsVolume > 10) CalloutsVolume = 10;

  Audio.SetMusicVolume(MusicVolume);
  Audio.SetSoundFXVolume(SoundEffectsVolume);
  Audio.SetNotificationsVolume(CalloutsVolume);

  TournamentScoring = (ReadSetting(EEPROM_TOURNAMENT_SCORING_BYTE, 0)) ? true : false;

  MaxTiltWarnings = ReadSetting(EEPROM_TILT_WARNING_BYTE, 2);
  if (MaxTiltWarnings > 2) MaxTiltWarnings = 2;

  byte awardOverride = ReadSetting(EEPROM_AWARD_OVERRIDE_BYTE, 99);
  if (awardOverride != 99) {
    ScoreAwardReplay = awardOverride;
  }

  byte ballsOverride = ReadSetting(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  if (ballsOverride == 3 || ballsOverride == 5) {
    BallsPerGame = ballsOverride;
  } else {
    if (ballsOverride != 99) EEPROM.write(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  }

  ScrollingScores = (ReadSetting(EEPROM_SCROLLING_SCORES_BYTE, 1)) ? true : false;

  ExtraBallValue = BSOS_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_BYTE);
  if (ExtraBallValue % 1000 || ExtraBallValue > 100000) ExtraBallValue = 20000;

  SpecialValue = BSOS_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_BYTE);
  if (SpecialValue % 1000 || SpecialValue > 100000) SpecialValue = 40000;

  DimLevel = ReadSetting(EEPROM_DIM_LEVEL_BYTE, 2);
  if (DimLevel < 2 || DimLevel > 3) DimLevel = 2;
  BSOS_SetDimDivisor(1, DimLevel);

  BigGameJackpotTime = ReadSetting(EEPROM_BG_JACKPOT_TIME, 10);
  if (BigGameJackpotTime%10 || BigGameJackpotTime>60) BigGameJackpotTime = 10;

  HurryUpDropLevel = ReadSetting(EEPROM_HUD_LEVEL, 1);
  if (HurryUpDropLevel>2 && HurryUpDropLevel!=99) HurryUpDropLevel = 1;

  LiberalClearRule = ReadSetting(EEPROM_HUD_LIBERAL, 1) ? true : false;

  MaxCardsLit = ReadSetting(EEPROM_MAX_CARDS_LIT, 1);
  if (MaxCardsLit>3) MaxCardsLit = 1;

  MaxCardsAffectedByLevel = ReadSetting(EEPROM_MAX_CARDS_DECREASES, 1) ? true : false;

  TimeRequiredToResetGame = ReadSetting(EEPROM_CRB_HOLD_TIME, 1);
  if (TimeRequiredToResetGame>3 && TimeRequiredToResetGame!=99) TimeRequiredToResetGame = 1;

  AwardScores[0] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_1_EEPROM_START_BYTE);
  AwardScores[1] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_2_EEPROM_START_BYTE);
  AwardScores[2] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_3_EEPROM_START_BYTE);

}




void setup() {
  if (DEBUG_MESSAGES) {
    Serial.begin(57600);
  }

  Serial.begin(57600);
  Serial.write("Hello world\n");
  // Tell the OS about game-specific lights and switches
  BSOS_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS, SolenoidAssociatedSwitches);

  // Set up the chips and interrupts
  BSOS_InitializeMPU();
  BSOS_DisableSolenoidStack();
  BSOS_SetDisableFlippers(true);

  // Read parameters from EEProm
  ReadStoredParameters();
  BSOS_SetCoinLockout((Credits >= MaximumCredits) ? true : false);

  CurrentScores[0] = BIG_GAME_2022_MAJOR_VERSION;
  CurrentScores[1] = BIG_GAME_2022_MINOR_VERSION;
  CurrentScores[2] = BALLY_STERN_OS_MAJOR_VERSION;
  CurrentScores[3] = BALLY_STERN_OS_MINOR_VERSION;

  CurrentTime = millis();
  Audio.InitDevices(AUDIO_PLAY_TYPE_WAV_TRIGGER | AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS);
  Audio.StopAllAudio();
  delayMicroseconds(10000);
  Audio.SetMusicDuckingGain(20);
  Audio.PlaySound(SOUND_EFFECT_MACHINE_START, AUDIO_PLAY_TYPE_WAV_TRIGGER);

  DropTargets123.DefineSwitch(0, SW_DROP_TARGET_1);
  DropTargets123.DefineSwitch(1, SW_DROP_TARGET_2);
  DropTargets123.DefineSwitch(2, SW_DROP_TARGET_3);
  DropTargets123.DefineResetSolenoid(0, SOL_BANK123_RESET);

  DropTargets456.DefineSwitch(0, SW_DROP_TARGET_4);
  DropTargets456.DefineSwitch(1, SW_DROP_TARGET_5);
  DropTargets456.DefineSwitch(2, SW_DROP_TARGET_6);
  DropTargets456.DefineResetSolenoid(0, SOL_BANK456_RESET);

  DropTargets789.DefineSwitch(0, SW_DROP_TARGET_7);
  DropTargets789.DefineSwitch(1, SW_DROP_TARGET_8);
  DropTargets789.DefineSwitch(2, SW_DROP_TARGET_9);
  DropTargets789.DefineResetSolenoid(0, SOL_BANK789_RESET);

  MachineState = MACHINE_STATE_ATTRACT;
}

byte ReadSetting(byte setting, byte defaultValue) {
  byte value = EEPROM.read(setting);
  if (value == 0xFF) {
    EEPROM.write(setting, defaultValue);
    return defaultValue;
  }
  return value;
}


// This function is useful for checking the status of drop target switches
byte CheckSequentialSwitches(byte startingSwitch, byte numSwitches) {
  byte returnSwitches = 0;
  for (byte count = 0; count < numSwitches; count++) {
    returnSwitches |= (BSOS_ReadSingleSwitchState(startingSwitch + count) << count);
  }
  return returnSwitches;
}



////////////////////////////////////////////////////////////////////////////
//
//  Lamp Management functions
//
////////////////////////////////////////////////////////////////////////////

void SetGeneralIllumination(boolean generalIlluminationOn) {

  if (generalIlluminationOn != GeneralIlluminationOn) {
    GeneralIlluminationOn = generalIlluminationOn;
    BSOS_SetContinuousSolenoidBit(GeneralIlluminationOn, SOLCONT_GENERAL_ILLUMINATION);
  }

}


byte XCardLamps[3][3] = {
  {LAMP_X1, LAMP_X2, LAMP_X3},
  {LAMP_X4, LAMP_X5, LAMP_X6},
  {LAMP_X7, LAMP_X8, LAMP_X9}
};

byte YCardLamps[3][3] = {
  {LAMP_Y8, LAMP_Y5, LAMP_Y6},
  {LAMP_Y9, LAMP_Y4, LAMP_Y7},
  {LAMP_Y2, LAMP_Y1, LAMP_Y3}
};

byte ZCardLamps[3][3] = {
  {LAMP_Z4, LAMP_Z7, LAMP_Z2},
  {LAMP_Z3, LAMP_Z9, LAMP_Z1},
  {LAMP_Z5, LAMP_Z6, LAMP_Z8}
};

// 0000 0987 6543 2198 7654 3219 8765 4321
unsigned long CardAnimation[12] = {
  0x00000000,
  0x00000800,
  0x00004C00,
  0x00026E00,
  0x00037E00,
  0x0003FE00,
  0x0003FE04,
  0x0013FE26,
  0x009BFF37,
  0x04DFFFBF,
  0x06FFFFFF,
  0x07FFFFFF,
};

void ShowCardLamps() {

  if (GameMode == GAME_MODE_SKILL_SHOT_PRE_SWITCH || GameMode == GAME_MODE_SKILL_SHOT_BUILD_BONUS) {
    /*
        for (byte cardRow=0; cardRow<3; cardRow++) {
          for (byte cardCol=0; cardCol<3; cardCol++) {
            BSOS_SetLampState(XCardLamps[cardRow][cardCol], Prey[CurrentPlayer].CheckPosition(0, cardRow, cardCol));
            BSOS_SetLampState(YCardLamps[cardRow][cardCol], Prey[CurrentPlayer].CheckPosition(1, cardRow, cardCol));
            BSOS_SetLampState(ZCardLamps[cardRow][cardCol], Prey[CurrentPlayer].CheckPosition(2, cardRow, cardCol));
          }
        }
    */
  } else {
    if (RightSpinnerAnimationStart) {
      BSOS_SetLampState(XCardLamps[0][0], 0);
      BSOS_SetLampState(XCardLamps[0][1], RightSpinnerAnimationPhase == 7 || RightSpinnerAnimationPhase == 8);
      BSOS_SetLampState(XCardLamps[0][2], 0);
      BSOS_SetLampState(XCardLamps[1][0], RightSpinnerAnimationPhase == 8 || RightSpinnerAnimationPhase == 9);
      BSOS_SetLampState(XCardLamps[1][1], 0);
      BSOS_SetLampState(XCardLamps[1][2], RightSpinnerAnimationPhase == 6 || RightSpinnerAnimationPhase == 7);
      BSOS_SetLampState(XCardLamps[2][0], 0);
      BSOS_SetLampState(XCardLamps[2][1], RightSpinnerAnimationPhase == 9 || RightSpinnerAnimationPhase == 10);
      BSOS_SetLampState(XCardLamps[2][2], RightSpinnerAnimationPhase == 5 || RightSpinnerAnimationPhase == 6);

      BSOS_SetLampState(YCardLamps[0][0], 0);
      BSOS_SetLampState(YCardLamps[0][1], RightSpinnerAnimationPhase == 7 || RightSpinnerAnimationPhase == 8);
      BSOS_SetLampState(YCardLamps[0][2], 0);
      BSOS_SetLampState(YCardLamps[1][0], RightSpinnerAnimationPhase == 6 || RightSpinnerAnimationPhase == 7);
      BSOS_SetLampState(YCardLamps[1][1], 0);
      BSOS_SetLampState(YCardLamps[1][2], RightSpinnerAnimationPhase == 8 || RightSpinnerAnimationPhase == 9);
      BSOS_SetLampState(YCardLamps[2][0], RightSpinnerAnimationPhase == 5 || RightSpinnerAnimationPhase == 6);
      BSOS_SetLampState(YCardLamps[2][1], RightSpinnerAnimationPhase == 9 || RightSpinnerAnimationPhase == 10);
      BSOS_SetLampState(YCardLamps[2][2], 0);

      BSOS_SetLampState(ZCardLamps[0][0], RightSpinnerAnimationPhase == 4 || RightSpinnerAnimationPhase == 5 || RightSpinnerAnimationPhase == 10 || RightSpinnerAnimationPhase == 11);
      BSOS_SetLampState(ZCardLamps[0][1], RightSpinnerAnimationPhase == 5 || RightSpinnerAnimationPhase == 6 || RightSpinnerAnimationPhase == 11 || RightSpinnerAnimationPhase == 12);
      BSOS_SetLampState(ZCardLamps[0][2], RightSpinnerAnimationPhase == 4 || RightSpinnerAnimationPhase == 5 || RightSpinnerAnimationPhase == 10 || RightSpinnerAnimationPhase == 11);
      BSOS_SetLampState(ZCardLamps[1][0], RightSpinnerAnimationPhase == 12 || RightSpinnerAnimationPhase == 13);
      BSOS_SetLampState(ZCardLamps[1][1], RightSpinnerAnimationPhase == 3 || RightSpinnerAnimationPhase == 4);
      BSOS_SetLampState(ZCardLamps[1][2], RightSpinnerAnimationPhase == 12 || RightSpinnerAnimationPhase == 13);
      BSOS_SetLampState(ZCardLamps[2][0], RightSpinnerAnimationPhase == 13 || RightSpinnerAnimationPhase == 14);
      BSOS_SetLampState(ZCardLamps[2][1], RightSpinnerAnimationPhase == 3);
      BSOS_SetLampState(ZCardLamps[2][2], RightSpinnerAnimationPhase == 13 || RightSpinnerAnimationPhase == 14);
    } else if (CardLevelIncreaseTime) {
      for (byte cardRow = 0; cardRow < 3; cardRow++) {
        for (byte cardCol = 0; cardCol < 3; cardCol++) {
          BSOS_SetLampState(XCardLamps[cardRow][cardCol], 1, 0, 500);
          BSOS_SetLampState(YCardLamps[cardRow][cardCol], 1, 0, 500);
          BSOS_SetLampState(ZCardLamps[cardRow][cardCol], 1, 0, 500);
        }
      }
    } else {
      if (HidePreyOnCards) {
        unsigned long statusBitMask = 0x01;
        for (byte cardRow = 0; cardRow < 3; cardRow++) {
          for (byte cardCol = 0; cardCol < 3; cardCol++) {
            boolean cardEntryHidden;

            cardEntryHidden = (CardStatus[CurrentPlayer] & statusBitMask) == 0;
            if (cardEntryHidden) BSOS_SetLampState(XCardLamps[cardRow][cardCol], 1, 1);
            else BSOS_SetLampState(XCardLamps[cardRow][cardCol], Prey[CurrentPlayer].CheckPosition(0, cardRow, cardCol), 0, 100);

            cardEntryHidden = (CardStatus[CurrentPlayer] & (statusBitMask * 512)) == 0;
            if (cardEntryHidden) BSOS_SetLampState(YCardLamps[cardRow][cardCol], 1, 1);
            else BSOS_SetLampState(YCardLamps[cardRow][cardCol], Prey[CurrentPlayer].CheckPosition(1, cardRow, cardCol), 0, 100);

            cardEntryHidden = (CardStatus[CurrentPlayer] & (statusBitMask * 262144)) == 0;
            if (cardEntryHidden) BSOS_SetLampState(ZCardLamps[cardRow][cardCol], 1, 1);
            else BSOS_SetLampState(ZCardLamps[cardRow][cardCol], Prey[CurrentPlayer].CheckPosition(2, cardRow, cardCol), 0, 100);

            statusBitMask *= 2;
          }
        }
      } else {
        unsigned long statusBitMask = 0x01;
        for (byte cardRow = 0; cardRow < 3; cardRow++) {
          for (byte cardCol = 0; cardCol < 3; cardCol++) {
            boolean cardSpotLit, preyInSpot;

            cardSpotLit = (CardStatus[CurrentPlayer] & statusBitMask) ? true : false;
            preyInSpot = Prey[CurrentPlayer].CheckPosition(0, cardRow, cardCol);
            BSOS_SetLampState(XCardLamps[cardRow][cardCol], preyInSpot || cardSpotLit, cardSpotLit & !preyInSpot, preyInSpot ? 100 : 0);

            cardSpotLit = (CardStatus[CurrentPlayer] & (statusBitMask * 512)) ? true : false;
            preyInSpot = Prey[CurrentPlayer].CheckPosition(1, cardRow, cardCol);
            BSOS_SetLampState(YCardLamps[cardRow][cardCol], preyInSpot || cardSpotLit, cardSpotLit & !preyInSpot, preyInSpot ? 100 : 0);

            cardSpotLit = (CardStatus[CurrentPlayer] & (statusBitMask * 262144)) ? true : false;
            preyInSpot = Prey[CurrentPlayer].CheckPosition(2, cardRow, cardCol);
            BSOS_SetLampState(ZCardLamps[cardRow][cardCol], preyInSpot || cardSpotLit, cardSpotLit & !preyInSpot, preyInSpot ? 100 : 0);

            statusBitMask *= 2;
          }
        }
      }
    }

    BSOS_SetLampState(LAMP_X_AND_LEFT_SPINNER, CardSelected[CurrentPlayer] & CARD_X_SELECTED);
    BSOS_SetLampState(LAMP_Y, CardSelected[CurrentPlayer] & CARD_Y_SELECTED);
    BSOS_SetLampState(LAMP_Z_AND_RIGHT_1K, CardSelected[CurrentPlayer] & CARD_Z_SELECTED);
    BSOS_SetLampState(LAMP_BONUS_RESERVE, CardLevel[CurrentPlayer], 0, (CardLevel[CurrentPlayer] > 1) ? (2000 / CardLevel[CurrentPlayer]) : 0);
  }


}

/*
  void ShowCardLamps() {
  if (GameMode==GAME_MODE_SKILL_SHOT_PRE_SWITCH) {
  } else if (GameMode==0xFF) {
    byte pinwheelStep = ((CurrentTime-GameModeStartTime)/150)%4;
    BSOS_SetLampState(XCardLamps[0][0], pinwheelStep==3);
    BSOS_SetLampState(XCardLamps[0][1], pinwheelStep==0);
    BSOS_SetLampState(XCardLamps[0][2], pinwheelStep==1);
    BSOS_SetLampState(XCardLamps[1][0], pinwheelStep==2);
    BSOS_SetLampState(XCardLamps[1][1], 1, 1);
    BSOS_SetLampState(XCardLamps[1][2], pinwheelStep==2);
    BSOS_SetLampState(XCardLamps[2][0], pinwheelStep==1);
    BSOS_SetLampState(XCardLamps[2][1], pinwheelStep==0);
    BSOS_SetLampState(XCardLamps[2][2], pinwheelStep==3);

    BSOS_SetLampState(YCardLamps[0][0], pinwheelStep==3);
    BSOS_SetLampState(YCardLamps[0][1], pinwheelStep==0);
    BSOS_SetLampState(YCardLamps[0][2], pinwheelStep==1);
    BSOS_SetLampState(YCardLamps[1][0], pinwheelStep==2);
    BSOS_SetLampState(YCardLamps[1][1], 1, 1);
    BSOS_SetLampState(YCardLamps[1][2], pinwheelStep==2);
    BSOS_SetLampState(YCardLamps[2][0], pinwheelStep==1);
    BSOS_SetLampState(YCardLamps[2][1], pinwheelStep==0);
    BSOS_SetLampState(YCardLamps[2][2], pinwheelStep==3);

    BSOS_SetLampState(ZCardLamps[0][0], pinwheelStep==3);
    BSOS_SetLampState(ZCardLamps[0][1], pinwheelStep==0);
    BSOS_SetLampState(ZCardLamps[0][2], pinwheelStep==1);
    BSOS_SetLampState(ZCardLamps[1][0], pinwheelStep==2);
    BSOS_SetLampState(ZCardLamps[1][1], 1, 1);
    BSOS_SetLampState(ZCardLamps[1][2], pinwheelStep==2);
    BSOS_SetLampState(ZCardLamps[2][0], pinwheelStep==1);
    BSOS_SetLampState(ZCardLamps[2][1], pinwheelStep==0);
    BSOS_SetLampState(ZCardLamps[2][2], pinwheelStep==3);
  } else if (GameMode==GAME_MODE_ALL_PREY_KILLED) {
    unsigned long statusBitMask = 0x01;
    for (byte cardRow=0; cardRow<3; cardRow++) {
      for (byte cardCol=0; cardCol<3; cardCol++) {

        if (CardScan&(statusBitMask)) BSOS_SetLampState(XCardLamps[cardRow][cardCol], 1);
        else if (CardWipe&(statusBitMask)) BSOS_SetLampState(XCardLamps[cardRow][cardCol], 0);
        else BSOS_SetLampState(XCardLamps[cardRow][cardCol], (CardStatus[CurrentPlayer]&statusBitMask)==0, 1);

        if (CardScan&(statusBitMask*512)) BSOS_SetLampState(YCardLamps[cardRow][cardCol], 1);
        else if (CardWipe&(statusBitMask*512)) BSOS_SetLampState(YCardLamps[cardRow][cardCol], 0);
        else BSOS_SetLampState(YCardLamps[cardRow][cardCol], (CardStatus[CurrentPlayer]&(statusBitMask*512))==0, 1);

        if (CardScan&(statusBitMask*262144)) BSOS_SetLampState(ZCardLamps[cardRow][cardCol], 1);
        else if (CardWipe&(statusBitMask*262144)) BSOS_SetLampState(ZCardLamps[cardRow][cardCol], 0);
        else BSOS_SetLampState(ZCardLamps[cardRow][cardCol], (CardStatus[CurrentPlayer]&(statusBitMask*262144))==0, 1);

        statusBitMask *= 2;
      }
    }
  } else if (GameMode==GAME_MODE_SKILL_SHOT_BUILD_BONUS || GameMode==GAME_MODE_UNSTRUCTURED_PLAY) {

    if (RevealCardsStartTime!=0) {
      int revealPhase = (CurrentTime-RevealCardsStartTime)/100;
      if (revealPhase<100) {
        for (byte cardRow=0; cardRow<3; cardRow++) {
          for (byte cardCol=0; cardCol<3; cardCol++) {
            BSOS_SetLampState(XCardLamps[cardRow][cardCol], Prey[CurrentPlayer].CheckPosition(0, cardRow, cardCol));
            BSOS_SetLampState(YCardLamps[cardRow][cardCol], Prey[CurrentPlayer].CheckPosition(1, cardRow, cardCol));
            BSOS_SetLampState(ZCardLamps[cardRow][cardCol], Prey[CurrentPlayer].CheckPosition(2, cardRow, cardCol));
          }
        }
      } else if (revealPhase<112) {
        unsigned long animationBitMask = 0x01;
        for (byte cardRow=0; cardRow<3; cardRow++) {
          for (byte cardCol=0; cardCol<3; cardCol++) {
            boolean dimXCard, dimYCard, dimZCard;
            dimXCard = (CardAnimation[revealPhase-100]&animationBitMask) ? true : false;
            dimYCard = (CardAnimation[revealPhase-100]&(animationBitMask*512)) ? true : false;
            dimZCard = (CardAnimation[revealPhase-100]&(animationBitMask*262144)) ? true : false;
            BSOS_SetLampState(XCardLamps[cardRow][cardCol], dimXCard || Prey[CurrentPlayer].CheckPosition(0, cardRow, cardCol), dimXCard, (!dimXCard)?100:0);
            BSOS_SetLampState(YCardLamps[cardRow][cardCol], dimYCard || Prey[CurrentPlayer].CheckPosition(1, cardRow, cardCol), dimYCard, (!dimYCard)?100:0);
            BSOS_SetLampState(ZCardLamps[cardRow][cardCol], dimZCard || Prey[CurrentPlayer].CheckPosition(2, cardRow, cardCol), dimZCard, (!dimZCard)?100:0);
            animationBitMask *= 2;
          }
        }
      } else {
        RevealCardsStartTime = 0;
      }
    } else {

      unsigned long statusBitMask = 0x01;
      for (byte cardRow=0; cardRow<3; cardRow++) {
        for (byte cardCol=0; cardCol<3; cardCol++) {
          boolean cardEntryHidden;

          cardEntryHidden = (CardStatus[CurrentPlayer]&statusBitMask)==0;
          if (cardEntryHidden) BSOS_SetLampState(XCardLamps[cardRow][cardCol], 1, 1);
          else BSOS_SetLampState(XCardLamps[cardRow][cardCol], Prey[CurrentPlayer].CheckPosition(0, cardRow, cardCol), 0, 100);

          cardEntryHidden = (CardStatus[CurrentPlayer]&(statusBitMask*512))==0;
          if (cardEntryHidden) BSOS_SetLampState(YCardLamps[cardRow][cardCol], 1, 1);
          else BSOS_SetLampState(YCardLamps[cardRow][cardCol], Prey[CurrentPlayer].CheckPosition(1, cardRow, cardCol), 0, 100);

          cardEntryHidden = (CardStatus[CurrentPlayer]&(statusBitMask*262144))==0;
          if (cardEntryHidden) BSOS_SetLampState(ZCardLamps[cardRow][cardCol], 1, 1);
          else BSOS_SetLampState(ZCardLamps[cardRow][cardCol], Prey[CurrentPlayer].CheckPosition(2, cardRow, cardCol), 0, 100);

          statusBitMask *= 2;
        }
      }

    }
    BSOS_SetLampState(LAMP_X_AND_LEFT_SPINNER, CardSelected[CurrentPlayer] & CARD_X_SELECTED);
    BSOS_SetLampState(LAMP_Y, CardSelected[CurrentPlayer] & CARD_Y_SELECTED);
    BSOS_SetLampState(LAMP_Z_AND_RIGHT_1K, CardSelected[CurrentPlayer] & CARD_Z_SELECTED);

  }
  }
*/


/*
   BonusX
   2 - 1 0
   3 - 0 1
   4 - F 0
   5 - 1 1
   6 - 0 F
   7 - F 1
   8 - 1 F
*/

void ShowBonusXLamps() {

  if (GameMode == GAME_MODE_SKILL_SHOT_PRE_SWITCH || GameMode == GAME_MODE_SKILL_SHOT_BUILD_BONUS) {
    /*
        BSOS_SetLampState(LAMP_BONUS_EXTRA_5K, 0);
        BSOS_SetLampState(LAMP_2X, 0);
        BSOS_SetLampState(LAMP_3X, 0);
    */
    //  } else if (RightSpinnerAnimationStart) {
    //    BSOS_SetLampState(LAMP_BONUS_EXTRA_5K, RightSpinnerAnimationPhase==2 || RightSpinnerAnimationPhase==3);
    //    BSOS_SetLampState(LAMP_2X, RightSpinnerAnimationPhase==1 || RightSpinnerAnimationPhase==2);
    //    BSOS_SetLampState(LAMP_3X, RightSpinnerAnimationPhase==1 || RightSpinnerAnimationPhase==2);
  } else {

    byte flash = 0;
    if (CurrentTime > (BonusXAnimationStart + 5000)) {
      BonusXAnimationStart = 0;
    }
    if (BonusXAnimationStart) {
      flash = 250;
    }

    boolean flash2x = BonusX == 4 || BonusX == 7;
    boolean flash3x = BonusX == 6 || BonusX == 8;
    if (flash) {
      BSOS_SetLampState(LAMP_2X, BonusX == 2 || BonusX == 4 || BonusX == 5 || BonusX == 7 || BonusX == 8, 0, flash);
      BSOS_SetLampState(LAMP_3X, BonusX == 3 || BonusX > 4, 0, flash);
    } else {
      BSOS_SetLampState(LAMP_2X, BonusX == 2 || BonusX == 4 || BonusX == 5 || BonusX == 7 || BonusX == 8, 0, flash2x ? 500 : 0);
      BSOS_SetLampState(LAMP_3X, BonusX == 3 || BonusX > 4, 0, flash3x ? 500 : 0);
    }

    BSOS_SetLampState(LAMP_BONUS_EXTRA_5K, CurrentBallInPlay >= BonusLineBall);
  }


}


void ShowShootAgainLamp() {

  if (!BallSaveUsed && CurrentBallSaveNumSeconds > 0 && (CurrentTime < BallSaveEndTime)) {
    unsigned long msRemaining = 5000;
    if (BallSaveEndTime != 0) msRemaining = BallSaveEndTime - CurrentTime;
    BSOS_SetLampState(LAMP_SHOOT_AGAIN, 1, 0, (msRemaining < 5000) ? 100 : 500);
  } else {
    BSOS_SetLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
  }
}


void ShowSaucerAndDropLamps() {

  if (GameMode == GAME_MODE_SKILL_SHOT_PRE_SWITCH || GameMode == GAME_MODE_SKILL_SHOT_BUILD_BONUS) {
    //    BSOS_SetLampState(LAMP_STANDUP_5K, 0);
  } else {
    int levelFlash = 0;
    if (BigGameLevel[CurrentPlayer] > 1) {
      levelFlash = 2000 / BigGameLevel[CurrentPlayer];
    }
    BSOS_SetLampState(LAMP_STANDUP_5K, BigGameLevel[CurrentPlayer], 0, levelFlash);

    if (Drop123HurryUp == 0) BSOS_SetLampState(LAMP_LEFT_BANK_25K, Drops123Done);
    if (Drop456HurryUp == 0) BSOS_SetLampState(LAMP_TOP_RIGHT_BANK_25K, Drops456Done);
    if (Drop789HurryUp == 0) BSOS_SetLampState(LAMP_LOWER_RIGHT_BANK_25K, Drops789Done);

  }
}




void ShowBIGGAMELamps() {

  if (GameMode == GAME_MODE_SKILL_SHOT_PRE_SWITCH || GameMode == GAME_MODE_SKILL_SHOT_BUILD_BONUS) {

    if (!(SkillShotLetter & BIG_GAME_LETTER_B)) BSOS_SetLampState(LAMP_B, BigGameStatus[CurrentPlayer]&BIG_GAME_LETTER_B);
    else BSOS_SetLampState(LAMP_B, SkillShotLetter & BIG_GAME_LETTER_B, 0, 400);
    if (!(SkillShotLetter & BIG_GAME_LETTER_I)) BSOS_SetLampState(LAMP_I, BigGameStatus[CurrentPlayer]&BIG_GAME_LETTER_I);
    else BSOS_SetLampState(LAMP_I, SkillShotLetter & BIG_GAME_LETTER_I, 0, 400);
    if (!(SkillShotLetter & BIG_GAME_LETTER_G1)) BSOS_SetLampState(LAMP_G1, BigGameStatus[CurrentPlayer]&BIG_GAME_LETTER_G1);
    else BSOS_SetLampState(LAMP_G1, SkillShotLetter & BIG_GAME_LETTER_G1, 0, 400);

    /*
        BSOS_SetLampState(LAMP_G2, 0);
        BSOS_SetLampState(LAMP_A, 0);
        BSOS_SetLampState(LAMP_M, 0);
        BSOS_SetLampState(LAMP_E, 0);
    */
  } else if (BIGGAMELevelIncreaseTime) {
    for (byte count = 0; count < 7; count++) {
      BSOS_SetLampState(LAMP_E + count, 1, 0, 500);
    }
  } else {
    if (AnimateSkillShotStartTime) {
      BSOS_SetLampState(LAMP_B, SkillShotLetter & BIG_GAME_LETTER_B, 0, 100);
      BSOS_SetLampState(LAMP_I, SkillShotLetter & BIG_GAME_LETTER_I, 0, 100);
      BSOS_SetLampState(LAMP_G1, SkillShotLetter & BIG_GAME_LETTER_G1, 0, 100);
      if (CurrentTime > (AnimateSkillShotStartTime + 3000)) AnimateSkillShotStartTime = 0;
    } else {
      BSOS_SetLampState(LAMP_B, BigGameStatus[CurrentPlayer]&BIG_GAME_LETTER_B);
      BSOS_SetLampState(LAMP_I, BigGameStatus[CurrentPlayer]&BIG_GAME_LETTER_I);
      BSOS_SetLampState(LAMP_G1, BigGameStatus[CurrentPlayer]&BIG_GAME_LETTER_G1);
    }

    BSOS_SetLampState(LAMP_G2, BigGameStatus[CurrentPlayer]&BIG_GAME_LETTER_G2, 0, (GameLampsHitTime[0]) ? 150 : 0);
    BSOS_SetLampState(LAMP_A, BigGameStatus[CurrentPlayer]&BIG_GAME_LETTER_A, 0, (GameLampsHitTime[1]) ? 150 : 0);
    BSOS_SetLampState(LAMP_M, BigGameStatus[CurrentPlayer]&BIG_GAME_LETTER_M, 0, (GameLampsHitTime[2]) ? 150 : 0);
    BSOS_SetLampState(LAMP_E, BigGameStatus[CurrentPlayer]&BIG_GAME_LETTER_E, 0, (GameLampsHitTime[3]) ? 150 : 0);
    for (byte count = 0; count < 4; count++) {
      if (CurrentTime > (GameLampsHitTime[count] + 3000)) GameLampsHitTime[count] = 0;
    }
  }

}


void ShowLaneLamps() {

  if (GameMode == GAME_MODE_SKILL_SHOT_PRE_SWITCH || GameMode == GAME_MODE_SKILL_SHOT_BUILD_BONUS) {
    /*
        BSOS_SetLampState(LAMP_SPECIAL_OUTLANE, 0);
        BSOS_SetLampState(LAMP_EXTRA_BALL_OUTLANE, 0);
        BSOS_SetLampState(LAMP_SPECIAL, 0);
        BSOS_SetLampState(LAMP_EXTRA_BALL, 0);
        BSOS_SetLampState(LAMP_TOP_I_5K, 0);
    */
  } else { /*if (GameMode==GAME_MODE_UNSTRUCTURED_PLAY) */
    BSOS_SetLampState(LAMP_SPECIAL_OUTLANE, LowerSpecial && !LowerSpecialCollected[CurrentPlayer]);
    BSOS_SetLampState(LAMP_EXTRA_BALL_OUTLANE, LowerExtraBall && !LowerExtraBallCollected[CurrentPlayer]);
    BSOS_SetLampState(LAMP_SPECIAL, 0);
    BSOS_SetLampState(LAMP_SPECIAL, BigGameLevel[CurrentPlayer] > 1 && BigGameAwarded[CurrentPlayer] < 2 && !UpperSpecialCollected[CurrentPlayer], 0, 200);
    BSOS_SetLampState(LAMP_EXTRA_BALL, BigGameLevel[CurrentPlayer] > 0 && BigGameAwarded[CurrentPlayer] == 0 && !UpperExtraBallCollected[CurrentPlayer], 0, 250);

    boolean top5KOn = false;
    if ((BigGameStatus[CurrentPlayer] & 0x70) == 0x70) top5KOn = true;

    int top5KFlash = 0;
    if (BIGHurryUpStart != 0) {
      top5KFlash = 500;
      if ((CurrentTime + 1000) > (BIGHurryUpStart + BIG_HURRY_UP_DURATION)) top5KFlash = 125;
    }
    BSOS_SetLampState(LAMP_TOP_I_5K, top5KOn, 0, top5KFlash);
  }
}


////////////////////////////////////////////////////////////////////////////
//
//  Display Management functions
//
////////////////////////////////////////////////////////////////////////////
unsigned long LastTimeScoreChanged = 0;
unsigned long LastFlashOrDash = 0;
unsigned long ScoreOverrideValue[4] = {0, 0, 0, 0};
byte LastAnimationSeed[4] = {0, 0, 0, 0};
byte AnimationStartSeed[4] = {0, 0, 0, 0};
byte ScoreOverrideStatus = 0;
byte ScoreAnimation[4] = {0, 0, 0, 0};
byte AnimationDisplayOrder[4] = {0, 1, 2, 3};
#define DISPLAY_OVERRIDE_BLANK_SCORE 0xFFFFFFFF
#define DISPLAY_OVERRIDE_ANIMATION_NONE     0
#define DISPLAY_OVERRIDE_ANIMATION_BOUNCE   1
#define DISPLAY_OVERRIDE_ANIMATION_FLUTTER  2
#define DISPLAY_OVERRIDE_ANIMATION_FLYBY    3
#define DISPLAY_OVERRIDE_ANIMATION_CENTER   4
byte LastScrollPhase = 0;

byte MagnitudeOfScore(unsigned long score) {
  if (score == 0) return 0;

  byte retval = 0;
  while (score > 0) {
    score = score / 10;
    retval += 1;
  }
  return retval;
}


void OverrideScoreDisplay(byte displayNum, unsigned long value, byte animationType) {
  if (displayNum > 3) return;

  ScoreOverrideStatus |= (0x01 << displayNum);
  ScoreAnimation[displayNum] = animationType;
  ScoreOverrideValue[displayNum] = value;
  LastAnimationSeed[displayNum] = 255;
}

byte GetDisplayMask(byte numDigits) {
  byte displayMask = 0;
  for (byte digitCount = 0; digitCount < numDigits; digitCount++) {
#ifdef BALLY_STERN_OS_USE_7_DIGIT_DISPLAYS
    displayMask |= (0x40 >> digitCount);
#else
    displayMask |= (0x20 >> digitCount);
#endif
  }
  return displayMask;
}


void SetAnimationDisplayOrder(byte disp0, byte disp1, byte disp2, byte disp3) {
  AnimationDisplayOrder[0] = disp0;
  AnimationDisplayOrder[1] = disp1;
  AnimationDisplayOrder[2] = disp2;
  AnimationDisplayOrder[3] = disp3;
}


void ShowAnimatedValue(byte displayNum, unsigned long displayScore, byte animationType) {
  byte overrideAnimationSeed;
  byte displayMask = BALLY_STERN_OS_ALL_DIGITS_MASK;

  byte numDigits = MagnitudeOfScore(displayScore);
  if (numDigits == 0) numDigits = 1;
  if (numDigits < (BALLY_STERN_OS_NUM_DIGITS - 1) && animationType == DISPLAY_OVERRIDE_ANIMATION_BOUNCE) {
    // This score is going to be animated (back and forth)
    overrideAnimationSeed = (CurrentTime / 250) % (2 * BALLY_STERN_OS_NUM_DIGITS - 2 * numDigits);
    if (overrideAnimationSeed != LastAnimationSeed[displayNum]) {

      LastAnimationSeed[displayNum] = overrideAnimationSeed;
      byte shiftDigits = (overrideAnimationSeed);
      if (shiftDigits >= ((BALLY_STERN_OS_NUM_DIGITS + 1) - numDigits)) shiftDigits = (BALLY_STERN_OS_NUM_DIGITS - numDigits) * 2 - shiftDigits;
      byte digitCount;
      displayMask = GetDisplayMask(numDigits);
      for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
        displayScore *= 10;
        displayMask = displayMask >> 1;
      }
      //BSOS_SetDisplayBlank(displayNum, 0x00);
      BSOS_SetDisplay(displayNum, displayScore, false);
      BSOS_SetDisplayBlank(displayNum, displayMask);
    }
  } else if (animationType == DISPLAY_OVERRIDE_ANIMATION_FLUTTER) {
    overrideAnimationSeed = CurrentTime / 50;
    if (overrideAnimationSeed != LastAnimationSeed[displayNum]) {
      LastAnimationSeed[displayNum] = overrideAnimationSeed;
      displayMask = GetDisplayMask(numDigits);
      if (overrideAnimationSeed % 2) {
        displayMask &= 0x55;
      } else {
        displayMask &= 0xAA;
      }
      BSOS_SetDisplay(displayNum, displayScore, false);
      BSOS_SetDisplayBlank(displayNum, displayMask);
    }
  } else if (animationType == DISPLAY_OVERRIDE_ANIMATION_FLYBY) {
    overrideAnimationSeed = (CurrentTime / 75) % 256;
    if (overrideAnimationSeed != LastAnimationSeed[displayNum]) {
      if (LastAnimationSeed[displayNum] == 255) {
        AnimationStartSeed[displayNum] = overrideAnimationSeed;
      }
      LastAnimationSeed[displayNum] = overrideAnimationSeed;

      byte realAnimationSeed = overrideAnimationSeed - AnimationStartSeed[displayNum];
      if (overrideAnimationSeed < AnimationStartSeed[displayNum]) realAnimationSeed = (255 - AnimationStartSeed[displayNum]) + overrideAnimationSeed;

      if (realAnimationSeed > 34) {
        BSOS_SetDisplayBlank(displayNum, 0x00);
        ScoreOverrideStatus &= ~(0x01 << displayNum);
      } else {
        int shiftDigits = (-6 * ((int)AnimationDisplayOrder[displayNum] + 1)) + realAnimationSeed;
        displayMask = GetDisplayMask(numDigits);
        if (shiftDigits < 0) {
          shiftDigits = 0 - shiftDigits;
          byte digitCount;
          for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
            displayScore /= 10;
            displayMask = displayMask << 1;
          }
        } else if (shiftDigits > 0) {
          byte digitCount;
          for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
            displayScore *= 10;
            displayMask = displayMask >> 1;
          }
        }
        BSOS_SetDisplay(displayNum, displayScore, false);
        BSOS_SetDisplayBlank(displayNum, displayMask);
      }
    }
  } else if (animationType == DISPLAY_OVERRIDE_ANIMATION_CENTER) {
    overrideAnimationSeed = CurrentTime / 250;
    if (overrideAnimationSeed != LastAnimationSeed[displayNum]) {
      LastAnimationSeed[displayNum] = overrideAnimationSeed;
      byte shiftDigits = (BALLY_STERN_OS_NUM_DIGITS - numDigits) / 2;

      byte digitCount;
      displayMask = GetDisplayMask(numDigits);
      for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
        displayScore *= 10;
        displayMask = displayMask >> 1;
      }
      //BSOS_SetDisplayBlank(displayNum, 0x00);
      BSOS_SetDisplay(displayNum, displayScore, false);
      BSOS_SetDisplayBlank(displayNum, displayMask);
    }
  } else {
    BSOS_SetDisplay(displayNum, displayScore, true, 1);
  }

}

void ShowPlayerScores(byte displayToUpdate, boolean flashCurrent, boolean dashCurrent, unsigned long allScoresShowValue = 0) {

  if (displayToUpdate == 0xFF) ScoreOverrideStatus = 0;
  byte displayMask = BALLY_STERN_OS_ALL_DIGITS_MASK;
  unsigned long displayScore = 0;
  byte scrollPhaseChanged = false;

  byte scrollPhase = ((CurrentTime - LastTimeScoreChanged) / 125) % 16;
  if (scrollPhase != LastScrollPhase) {
    LastScrollPhase = scrollPhase;
    scrollPhaseChanged = true;
  }

  for (byte scoreCount = 0; scoreCount < 4; scoreCount++) {

    // If this display is currently being overriden, then we should update it
    if (allScoresShowValue == 0 && (ScoreOverrideStatus & (0x01 << scoreCount))) {
      displayScore = ScoreOverrideValue[scoreCount];
      if (displayScore != DISPLAY_OVERRIDE_BLANK_SCORE) {
        ShowAnimatedValue(scoreCount, displayScore, ScoreAnimation[scoreCount]);
      } else {
        BSOS_SetDisplayBlank(scoreCount, 0);
      }

    } else {
      boolean showingCurrentAchievement = false;
      // No override, update scores designated by displayToUpdate
      if (allScoresShowValue == 0) {
        displayScore = CurrentScores[scoreCount];
        displayScore += (CurrentAchievements[scoreCount] % 10);
        if (CurrentAchievements[scoreCount]) showingCurrentAchievement = true;
      }
      else displayScore = allScoresShowValue;

      // If we're updating all displays, or the one currently matching the loop, or if we have to scroll
      if (displayToUpdate == 0xFF || displayToUpdate == scoreCount || displayScore > BALLY_STERN_OS_MAX_DISPLAY_SCORE || showingCurrentAchievement) {

        // Don't show this score if it's not a current player score (even if it's scrollable)
        if (displayToUpdate == 0xFF && (scoreCount >= CurrentNumPlayers && CurrentNumPlayers != 0) && allScoresShowValue == 0) {
          BSOS_SetDisplayBlank(scoreCount, 0x00);
          continue;
        }

        if (displayScore > BALLY_STERN_OS_MAX_DISPLAY_SCORE) {
          // Score needs to be scrolled
          if ((CurrentTime - LastTimeScoreChanged) < 2000) {
            // show score for four seconds after change
            BSOS_SetDisplay(scoreCount, displayScore % (BALLY_STERN_OS_MAX_DISPLAY_SCORE + 1), false);
            byte blank = BALLY_STERN_OS_ALL_DIGITS_MASK;
            if (showingCurrentAchievement && (CurrentTime / 200) % 2) {
              blank &= ~(0x01 << (BALLY_STERN_OS_NUM_DIGITS - 1));
            }
            BSOS_SetDisplayBlank(scoreCount, blank);
          } else {
            // Scores are scrolled 10 digits and then we wait for 6
            if (scrollPhase < 11 && scrollPhaseChanged) {
              byte numDigits = MagnitudeOfScore(displayScore);

              // Figure out top part of score
              unsigned long tempScore = displayScore;
              if (scrollPhase < BALLY_STERN_OS_NUM_DIGITS) {
                displayMask = BALLY_STERN_OS_ALL_DIGITS_MASK;
                for (byte scrollCount = 0; scrollCount < scrollPhase; scrollCount++) {
                  displayScore = (displayScore % (BALLY_STERN_OS_MAX_DISPLAY_SCORE + 1)) * 10;
                  displayMask = displayMask >> 1;
                }
              } else {
                displayScore = 0;
                displayMask = 0x00;
              }

              // Add in lower part of score
              if ((numDigits + scrollPhase) > 10) {
                byte numDigitsNeeded = (numDigits + scrollPhase) - 10;
                for (byte scrollCount = 0; scrollCount < (numDigits - numDigitsNeeded); scrollCount++) {
                  tempScore /= 10;
                }
                displayMask |= GetDisplayMask(MagnitudeOfScore(tempScore));
                displayScore += tempScore;
              }
              BSOS_SetDisplayBlank(scoreCount, displayMask);
              BSOS_SetDisplay(scoreCount, displayScore);
            }
          }
        } else {
          if (flashCurrent && displayToUpdate == scoreCount) {
            unsigned long flashSeed = CurrentTime / 250;
            if (flashSeed != LastFlashOrDash) {
              LastFlashOrDash = flashSeed;
              if (((CurrentTime / 250) % 2) == 0) BSOS_SetDisplayBlank(scoreCount, 0x00);
              else BSOS_SetDisplay(scoreCount, displayScore, true, 2);
            }
          } else if (dashCurrent && displayToUpdate == scoreCount) {
            unsigned long dashSeed = CurrentTime / 50;
            if (dashSeed != LastFlashOrDash) {
              LastFlashOrDash = dashSeed;
              byte dashPhase = (CurrentTime / 60) % (2 * BALLY_STERN_OS_NUM_DIGITS * 3);
              byte numDigits = MagnitudeOfScore(displayScore);
              if (dashPhase < (2 * BALLY_STERN_OS_NUM_DIGITS)) {
                displayMask = GetDisplayMask((numDigits == 0) ? 2 : numDigits);
                if (dashPhase < (BALLY_STERN_OS_NUM_DIGITS + 1)) {
                  for (byte maskCount = 0; maskCount < dashPhase; maskCount++) {
                    displayMask &= ~(0x01 << maskCount);
                  }
                } else {
                  for (byte maskCount = (2 * BALLY_STERN_OS_NUM_DIGITS); maskCount > dashPhase; maskCount--) {
                    byte firstDigit = (0x20) << (BALLY_STERN_OS_NUM_DIGITS - 6);
                    displayMask &= ~(firstDigit >> (maskCount - dashPhase - 1));
                  }
                }
                BSOS_SetDisplay(scoreCount, displayScore);
                BSOS_SetDisplayBlank(scoreCount, displayMask);
              } else {
                BSOS_SetDisplay(scoreCount, displayScore, true, 2);
              }
            }
          } else {
            byte blank;
            blank = BSOS_SetDisplay(scoreCount, displayScore, false, 2);
            if (showingCurrentAchievement && (CurrentTime / 200) % 2) {
              blank &= ~(0x01 << (BALLY_STERN_OS_NUM_DIGITS - 1));
            }
            BSOS_SetDisplayBlank(scoreCount, blank);
          }
        }
      } // End if this display should be updated
    } // End on non-overridden
  } // End loop on scores

}

void ShowFlybyValue(byte numToShow, unsigned long timeBase) {
  byte shiftDigits = (CurrentTime - timeBase) / 120;
  byte rightSideBlank = 0;

  unsigned long bigVersionOfNum = (unsigned long)numToShow;
  for (byte count = 0; count < shiftDigits; count++) {
    bigVersionOfNum *= 10;
    rightSideBlank /= 2;
    if (count > 2) rightSideBlank |= 0x20;
  }
  bigVersionOfNum /= 1000;

  byte curMask = BSOS_SetDisplay(CurrentPlayer, bigVersionOfNum, false, 0);
  if (bigVersionOfNum == 0) curMask = 0;
  BSOS_SetDisplayBlank(CurrentPlayer, ~(~curMask | rightSideBlank));
}


void StartScoreAnimation(unsigned long scoreToAnimate, boolean playTick = true) {
  if (ScoreAdditionAnimation != 0) {
    CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
  }
  ScoreAdditionAnimation = scoreToAnimate;
  ScoreAdditionAnimationStartTime = CurrentTime;
  LastRemainingAnimatedScoreShown = 0;
  PlayScoreAnimationTick = playTick;
}



////////////////////////////////////////////////////////////////////////////
//
//  Machine State Helper functions
//
////////////////////////////////////////////////////////////////////////////
boolean AddPlayer(boolean resetNumPlayers = false) {

  if (Credits < 1 && !FreePlayMode) return false;
  if (resetNumPlayers) CurrentNumPlayers = 0;
  if (CurrentNumPlayers >= 4) return false;

  CurrentNumPlayers += 1;
  BSOS_SetDisplay(CurrentNumPlayers - 1, 0, true, 2);
  //  BSOS_SetDisplayBlank(CurrentNumPlayers - 1, 0x30);

  if (!FreePlayMode) {
    Credits -= 1;
    BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
    BSOS_SetDisplayCredits(Credits, !FreePlayMode);
    BSOS_SetCoinLockout(false);
  }
  QueueNotification(SOUND_EFFECT_VP_ADD_PLAYER_1 + (CurrentNumPlayers - 1), 10);

  BSOS_WriteULToEEProm(BSOS_TOTAL_PLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_PLAYS_EEPROM_START_BYTE) + 1);

  return true;
}


unsigned short ChuteAuditByte[] = {BSOS_CHUTE_1_COINS_START_BYTE, BSOS_CHUTE_2_COINS_START_BYTE, BSOS_CHUTE_3_COINS_START_BYTE};
void AddCoinToAudit(byte chuteNum) {
  if (chuteNum > 2) return;
  unsigned short coinAuditStartByte = ChuteAuditByte[chuteNum];
  BSOS_WriteULToEEProm(coinAuditStartByte, BSOS_ReadULFromEEProm(coinAuditStartByte) + 1);
}


void AddCredit(boolean playSound = false, byte numToAdd = 1) {
  if (Credits < MaximumCredits) {
    Credits += numToAdd;
    if (Credits > MaximumCredits) Credits = MaximumCredits;
    BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
    if (playSound) PlaySoundEffect(SOUND_EFFECT_LETTER_ROLLOVER);
    BSOS_SetDisplayCredits(Credits, !FreePlayMode);
    BSOS_SetCoinLockout(false);
  } else {
    BSOS_SetDisplayCredits(Credits, !FreePlayMode);
    BSOS_SetCoinLockout(true);
  }

}


byte SwitchToChuteNum(byte switchHit) {
  byte chuteNum = 0;
  if (switchHit == SW_COIN_2) chuteNum = 1;
  else if (switchHit == SW_COIN_3) chuteNum = 2;
  return chuteNum;
}


boolean AddCoin(byte chuteNum) {
  boolean creditAdded = false;
  if (chuteNum > 2) return false;
  byte cpcSelection = GetCPCSelection(chuteNum);

  // Find the lowest chute num with the same ratio selection
  // and use that ChuteCoinsInProgress counter
  byte chuteNumToUse;
  for (chuteNumToUse = 0; chuteNumToUse <= chuteNum; chuteNumToUse++) {
    if (GetCPCSelection(chuteNumToUse) == cpcSelection) break;
  }

  PlaySoundEffect(SOUND_EFFECT_COIN_DROP_1 + (CurrentTime % 3));

  byte cpcCoins = GetCPCCoins(cpcSelection);
  byte cpcCredits = GetCPCCredits(cpcSelection);
  byte coinProgressBefore = ChuteCoinsInProgress[chuteNumToUse];
  ChuteCoinsInProgress[chuteNumToUse] += 1;

  if (ChuteCoinsInProgress[chuteNumToUse] == cpcCoins) {
    if (cpcCredits > cpcCoins) AddCredit(cpcCredits - (coinProgressBefore));
    else AddCredit(cpcCredits);
    ChuteCoinsInProgress[chuteNumToUse] = 0;
    creditAdded = true;
  } else {
    if (cpcCredits > cpcCoins) {
      AddCredit(1);
      creditAdded = true;
    } else {
    }
  }

  return creditAdded;
}

void AddSpecialCredit() {
  AddCredit(false, 1);
  BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
  BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
}

void AwardSpecial(boolean upper) {
  if (upper) {
    if (UpperSpecialCollected[CurrentPlayer]) return;
    UpperSpecialCollected[CurrentPlayer] = true;
  } else {
    if (LowerSpecialCollected[CurrentPlayer]) return;
    LowerSpecialCollected[CurrentPlayer] = true;
  }
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * SpecialValue;
  } else {
    AddSpecialCredit();
  }
}

void AwardExtraBall(boolean upper) {
  if (upper) {
    if (UpperExtraBallCollected[CurrentPlayer]) return;
    UpperExtraBallCollected[CurrentPlayer] = true;
  } else {
    if (LowerExtraBallCollected[CurrentPlayer]) return;
    LowerExtraBallCollected[CurrentPlayer] = true;
  }
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * ExtraBallValue;
  } else {
    SamePlayerShootsAgain = true;
    BSOS_SetLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
    PlaySoundEffect(SOUND_EFFECT_EXTRA_BALL);
  }
}




////////////////////////////////////////////////////////////////////////////
//
//  Audio Output functions
//
////////////////////////////////////////////////////////////////////////////

void QueueNotification(unsigned int soundEffectNum, byte priority) {
  if (CalloutsVolume == 0) return;
  if (SoundSelector < 3 || SoundSelector == 4 || SoundSelector == 7 || SoundSelector == 9) return;
  if (soundEffectNum < SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START || soundEffectNum >= (SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START + NUM_VOICE_NOTIFICATIONS)) return;

  if (DEBUG_MESSAGES) {
    char buf[256]; 
    sprintf(buf, "Notification %d\n", soundEffectNum);
    Serial.write(buf);
  }

  Audio.QueueNotification(soundEffectNum, VoiceNotificationDurations[soundEffectNum - SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START], priority, CurrentTime);
}

void PlaySoundEffect(unsigned int soundEffectNum) {

  if (SoundSelector == 0) return;

  if (SoundSelector > 5) Audio.PlaySound(soundEffectNum, AUDIO_PLAY_TYPE_WAV_TRIGGER);

  if (DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "Playing %d\n", soundEffectNum);
    Serial.write(buf);
  }

#ifdef BALLY_STERN_OS_USE_SB300
  switch (soundEffectNum) {
    case SOUND_EFFECT_VP_PLAYER_1_UP:
      /*
            AddToSoundQueue(SB300_SOUND_FUNCTION_SQUARE_WAVE, 1, 0x12, CurrentTime);
            AddToSoundQueue(SB300_SOUND_FUNCTION_SQUARE_WAVE, 0, 0x92, CurrentTime);
            AddToSoundQueue(SB300_SOUND_FUNCTION_SQUARE_WAVE, 6, 0x80, CurrentTime);
            AddToSoundQueue(SB300_SOUND_FUNCTION_SQUARE_WAVE, 7, 0x00, CurrentTime);
            AddToSoundQueue(SB300_SOUND_FUNCTION_ANALOG, 0, 0xAA, CurrentTime);
            AddToSoundQueue(SB300_SOUND_FUNCTION_ANALOG, 0, 0x8A, CurrentTime+50);
            AddToSoundQueue(SB300_SOUND_FUNCTION_ANALOG, 0, 0x6A, CurrentTime+100);
            AddToSoundQueue(SB300_SOUND_FUNCTION_ANALOG, 0, 0x4A, CurrentTime+150);
            AddToSoundQueue(SB300_SOUND_FUNCTION_ANALOG, 0, 0x2A, CurrentTime+250);
            AddToSoundQueue(SB300_SOUND_FUNCTION_ANALOG, 0, 0x0A, CurrentTime+300);
      */
      break;
  }
#endif

}



////////////////////////////////////////////////////////////////////////////
//
//  Self test, audit, adjustments mode
//
////////////////////////////////////////////////////////////////////////////

#define ADJ_TYPE_LIST                 1
#define ADJ_TYPE_MIN_MAX              2
#define ADJ_TYPE_MIN_MAX_DEFAULT      3
#define ADJ_TYPE_SCORE                4
#define ADJ_TYPE_SCORE_WITH_DEFAULT   5
#define ADJ_TYPE_SCORE_NO_DEFAULT     6
byte AdjustmentType = 0;
byte NumAdjustmentValues = 0;
byte AdjustmentValues[8];
unsigned long AdjustmentScore;
byte *CurrentAdjustmentByte = NULL;
unsigned long *CurrentAdjustmentUL = NULL;
byte CurrentAdjustmentStorageByte = 0;
byte TempValue = 0;

int RunSelfTest(int curState, boolean curStateChanged) {
  int returnState = curState;
  CurrentNumPlayers = 0;

  if (curStateChanged) {
    // Send a stop-all command and reset the sample-rate offset, in case we have
    //  reset while the WAV Trigger was already playing.
    Audio.StopAllAudio();
    int modeMapping = SelfTestStateToCalloutMap[-1 - curState];
    Audio.PlaySound((unsigned short)modeMapping, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
    SoundSettingTimeout = 0;
  } else {
    if (SoundSettingTimeout && CurrentTime > SoundSettingTimeout) {
      SoundSettingTimeout = 0;
      Audio.StopAllAudio();
    }
  }

  // Any state that's greater than CHUTE_3 is handled by the Base Self-test code
  // Any that's less, is machine specific, so we handle it here.
  if (curState >= MACHINE_STATE_TEST_DONE) {
    byte cpcSelection = 0xFF;
    byte chuteNum = 0xFF;
    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_1) chuteNum = 0;
    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_2) chuteNum = 1;
    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_3) chuteNum = 2;
    if (chuteNum!=0xFF) cpcSelection = GetCPCSelection(chuteNum);
    returnState = RunBaseSelfTest(returnState, curStateChanged, CurrentTime, SW_CREDIT_RESET, SW_SLAM);
    if (chuteNum!=0xFF) {
      if (cpcSelection != GetCPCSelection(chuteNum)) {
        byte newCPC = GetCPCSelection(chuteNum);
        Audio.StopAllAudio();
        PlaySoundEffect(SOUND_EFFECT_SELF_TEST_CPC_START+newCPC);
      }
    }
  } else {
    byte curSwitch = BSOS_PullFirstFromSwitchStack();

    if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
      SetLastSelfTestChangedTime(CurrentTime);
      returnState -= 1;
    }

    if (curSwitch == SW_SLAM) {
      returnState = MACHINE_STATE_ATTRACT;
    }

    if (curStateChanged) {
      for (int count = 0; count < 4; count++) {
        BSOS_SetDisplay(count, 0);
        BSOS_SetDisplayBlank(count, 0x00);
      }
      BSOS_SetDisplayCredits(MACHINE_STATE_TEST_SOUNDS - curState);
      BSOS_SetDisplayBallInPlay(0, false);
      CurrentAdjustmentByte = NULL;
      CurrentAdjustmentUL = NULL;
      CurrentAdjustmentStorageByte = 0;

      AdjustmentType = ADJ_TYPE_MIN_MAX;
      AdjustmentValues[0] = 0;
      AdjustmentValues[1] = 1;
      TempValue = 0;

      switch (curState) {
        case MACHINE_STATE_ADJUST_FREEPLAY:
          CurrentAdjustmentByte = (byte *)&FreePlayMode;
          CurrentAdjustmentStorageByte = EEPROM_FREE_PLAY_BYTE;
          break;
        case MACHINE_STATE_ADJUST_BALL_SAVE:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 5;
          AdjustmentValues[1] = 5;
          AdjustmentValues[2] = 10;
          AdjustmentValues[3] = 15;
          AdjustmentValues[4] = 20;
          CurrentAdjustmentByte = &BallSaveNumSeconds;
          CurrentAdjustmentStorageByte = EEPROM_BALL_SAVE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SFX_AND_SOUNDTRACK:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 5;
          AdjustmentValues[0] = 0;
          AdjustmentValues[1] = 6;
          AdjustmentValues[2] = 7;
          AdjustmentValues[3] = 8;
          AdjustmentValues[4] = 9;
          CurrentAdjustmentByte = &SoundSelector;
          CurrentAdjustmentStorageByte = EEPROM_SOUND_SELECTOR_BYTE;
          break;

        case MACHINE_STATE_ADJUST_MUSIC_VOLUME:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 1;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &MusicVolume;
          CurrentAdjustmentStorageByte = EEPROM_MUSIC_VOLUME_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SFX_VOLUME:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 1;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &SoundEffectsVolume;
          CurrentAdjustmentStorageByte = EEPROM_SFX_VOLUME_BYTE;
          break;
        case MACHINE_STATE_ADJUST_CALLOUTS_VOLUME:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 1;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &CalloutsVolume;
          CurrentAdjustmentStorageByte = EEPROM_CALLOUTS_VOLUME_BYTE;
          break;

        case MACHINE_STATE_ADJUST_TOURNAMENT_SCORING:
          CurrentAdjustmentByte = (byte *)&TournamentScoring;
          CurrentAdjustmentStorageByte = EEPROM_TOURNAMENT_SCORING_BYTE;
          break;
        case MACHINE_STATE_ADJUST_TILT_WARNING:
          AdjustmentValues[1] = 2;
          CurrentAdjustmentByte = &MaxTiltWarnings;
          CurrentAdjustmentStorageByte = EEPROM_TILT_WARNING_BYTE;
          break;
        case MACHINE_STATE_ADJUST_AWARD_OVERRIDE:
          AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
          AdjustmentValues[1] = 7;
          CurrentAdjustmentByte = &ScoreAwardReplay;
          CurrentAdjustmentStorageByte = EEPROM_AWARD_OVERRIDE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_BALLS_OVERRIDE:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 2;
          AdjustmentValues[0] = 3;
          AdjustmentValues[1] = 5;
          CurrentAdjustmentByte = &BallsPerGame;
          CurrentAdjustmentStorageByte = EEPROM_BALLS_OVERRIDE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SCROLLING_SCORES:
          CurrentAdjustmentByte = (byte *)&ScrollingScores;
          CurrentAdjustmentStorageByte = EEPROM_SCROLLING_SCORES_BYTE;
          break;

        case MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &ExtraBallValue;
          CurrentAdjustmentStorageByte = EEPROM_EXTRA_BALL_SCORE_BYTE;
          break;

        case MACHINE_STATE_ADJUST_SPECIAL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &SpecialValue;
          CurrentAdjustmentStorageByte = EEPROM_SPECIAL_SCORE_BYTE;
          break;

        case MACHINE_STATE_ADJUST_BIG_GAME_JACKPOT_TIME:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 7;
          AdjustmentValues[0] = 0;
          AdjustmentValues[1] = 10;
          AdjustmentValues[2] = 20;
          AdjustmentValues[3] = 30;
          AdjustmentValues[4] = 40;
          AdjustmentValues[5] = 50;
          AdjustmentValues[6] = 60;
          CurrentAdjustmentByte = &BigGameJackpotTime;
          CurrentAdjustmentStorageByte = EEPROM_BG_JACKPOT_TIME;
          break;

        case MACHINE_STATE_ADJUST_HURRY_UP_DROP_LEVEL:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 4;
          AdjustmentValues[0] = 0;
          AdjustmentValues[1] = 1;
          AdjustmentValues[2] = 2;
          AdjustmentValues[3] = 99;
          CurrentAdjustmentByte = &HurryUpDropLevel;
          CurrentAdjustmentStorageByte = EEPROM_HUD_LEVEL;
          break;

        case MACHINE_STATE_ADJUST_HURRY_UP_DROPS_LIBERAL:
          CurrentAdjustmentByte = (byte *)&LiberalClearRule;
          CurrentAdjustmentStorageByte = EEPROM_HUD_LIBERAL;
          break;

        case MACHINE_STATE_ADJUST_MAX_CARDS_LIT:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 1;
          AdjustmentValues[1] = 3;
          CurrentAdjustmentByte = &MaxCardsLit;
          CurrentAdjustmentStorageByte = EEPROM_MAX_CARDS_LIT;
          break;

        case MACHINE_STATE_ADJUST_MAX_CARDS_LIT_DECREASES:
          CurrentAdjustmentByte = (byte *)&LiberalClearRule;
          CurrentAdjustmentStorageByte = EEPROM_MAX_CARDS_DECREASES;
          break;

        case MACHINE_STATE_ADJUST_CREDIT_RESET_HOLD_TIME:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 5;
          AdjustmentValues[0] = 0;
          AdjustmentValues[1] = 1;
          AdjustmentValues[2] = 2;
          AdjustmentValues[3] = 3;
          AdjustmentValues[4] = 99;
          CurrentAdjustmentByte = &TimeRequiredToResetGame;
          CurrentAdjustmentStorageByte = EEPROM_CRB_HOLD_TIME; 
          break;
          
/*
        case MACHINE_STATE_ADJUST_DIM_LEVEL:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 2;
          AdjustmentValues[0] = 2;
          AdjustmentValues[1] = 3;
          CurrentAdjustmentByte = &DimLevel;
          CurrentAdjustmentStorageByte = EEPROM_DIM_LEVEL_BYTE;
          //          for (int count = 0; count < 7; count++) BSOS_SetLampState(MIDDLE_ROCKET_7K + count, 1, 1);
          break;
*/
        case MACHINE_STATE_ADJUST_DONE:
          returnState = MACHINE_STATE_ATTRACT;
          break;
      }

    }

    // Change value, if the switch is hit
    if (curSwitch == SW_CREDIT_RESET) {

      if (CurrentAdjustmentByte && (AdjustmentType == ADJ_TYPE_MIN_MAX || AdjustmentType == ADJ_TYPE_MIN_MAX_DEFAULT)) {
        byte curVal = *CurrentAdjustmentByte;
        curVal += 1;
        if (curVal > AdjustmentValues[1]) {
          if (AdjustmentType == ADJ_TYPE_MIN_MAX) curVal = AdjustmentValues[0];
          else {
            if (curVal > 99) curVal = AdjustmentValues[0];
            else curVal = 99;
          }
        }
        *CurrentAdjustmentByte = curVal;
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, curVal);

        if (curState == MACHINE_STATE_ADJUST_MUSIC_VOLUME) {
          if (SoundSettingTimeout) Audio.StopAllAudio();
          Audio.PlaySound(SOUND_EFFECT_BACKGROUND_SONG_1, AUDIO_PLAY_TYPE_WAV_TRIGGER, curVal);
          Audio.SetMusicVolume(curVal);
          SoundSettingTimeout = CurrentTime + 5000;
        } else if (curState == MACHINE_STATE_ADJUST_SFX_VOLUME) {
          if (SoundSettingTimeout) Audio.StopAllAudio();
          Audio.PlaySound(SOUND_EFFECT_DROP_TARGET_HIT, AUDIO_PLAY_TYPE_WAV_TRIGGER, curVal);
          Audio.SetSoundFXVolume(curVal);
          SoundSettingTimeout = CurrentTime + 5000;
        } else if (curState == MACHINE_STATE_ADJUST_CALLOUTS_VOLUME) {
          if (SoundSettingTimeout) Audio.StopAllAudio();
          Audio.PlaySound(SOUND_EFFECT_VP_SHOOT_AGAIN, AUDIO_PLAY_TYPE_WAV_TRIGGER, curVal);
          Audio.SetNotificationsVolume(curVal);
          SoundSettingTimeout = CurrentTime + 3000;
        }
      } else if (CurrentAdjustmentByte && AdjustmentType == ADJ_TYPE_LIST) {
        byte valCount = 0;
        byte curVal = *CurrentAdjustmentByte;
        byte newIndex = 0;
        for (valCount = 0; valCount < (NumAdjustmentValues - 1); valCount++) {
          if (curVal == AdjustmentValues[valCount]) newIndex = valCount + 1;
        }
        *CurrentAdjustmentByte = AdjustmentValues[newIndex];
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, AdjustmentValues[newIndex]);
      } else if (CurrentAdjustmentUL && (AdjustmentType == ADJ_TYPE_SCORE_WITH_DEFAULT || AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT)) {
        unsigned long curVal = *CurrentAdjustmentUL;
        curVal += 5000;
        if (curVal > 100000) curVal = 0;
        if (AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT && curVal == 0) curVal = 5000;
        *CurrentAdjustmentUL = curVal;
        if (CurrentAdjustmentStorageByte) BSOS_WriteULToEEProm(CurrentAdjustmentStorageByte, curVal);
      }

/*
      if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
        BSOS_SetDimDivisor(1, DimLevel);
      }
*/
      if (curState == MACHINE_STATE_ADJUST_SFX_AND_SOUNDTRACK) {
        Audio.StopAllAudio();
        PlaySoundEffect(SOUND_EFFECT_SELF_TEST_AUDIO_OPTIONS_START + *CurrentAdjustmentByte);
      } else if (curState == MACHINE_STATE_ADJUST_HURRY_UP_DROP_LEVEL) {
        Audio.StopAllAudio();
        if (*CurrentAdjustmentByte!=99) PlaySoundEffect(SOUND_EFFECT_AP_HUD_OPTION_1 + *CurrentAdjustmentByte);
        else PlaySoundEffect(SOUND_EFFECT_AP_HUD_OPTION_99);
      } else if (curState == MACHINE_STATE_ADJUST_CREDIT_RESET_HOLD_TIME) {
        Audio.StopAllAudio();
        if (*CurrentAdjustmentByte!=99) PlaySoundEffect(SOUND_EFFECT_AP_CRB_OPTION_1 + *CurrentAdjustmentByte);
        else PlaySoundEffect(SOUND_EFFECT_AP_CRB_OPTION_99);
      }
      
    }

    // Show current value
    if (CurrentAdjustmentByte != NULL) {
      BSOS_SetDisplay(0, (unsigned long)(*CurrentAdjustmentByte), true);
    } else if (CurrentAdjustmentUL != NULL) {
      BSOS_SetDisplay(0, (*CurrentAdjustmentUL), true);
    }

  }

/*
  if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
    //    for (int count = 0; count < 7; count++) BSOS_SetLampState(MIDDLE_ROCKET_7K + count, 1, (CurrentTime / 1000) % 2);
  }
*/  

  if (returnState == MACHINE_STATE_ATTRACT) {
    // If any variables have been set to non-override (99), return
    // them to dip switch settings
    // Balls Per Game, Player Loses On Ties, Novelty Scoring, Award Score
    //    DecodeDIPSwitchParameters();
    BSOS_SetDisplayCredits(Credits, !FreePlayMode);
    ReadStoredParameters();
  }

  return returnState;
}



////////////////////////////////////////////////////////////////////////////
//
//  Attract Mode
//
////////////////////////////////////////////////////////////////////////////

unsigned long AttractLastLadderTime = 0;
byte AttractLastLadderBonus = 0;
unsigned long AttractDisplayRampStart = 0;
byte AttractLastHeadMode = 255;
byte AttractLastPlayfieldMode = 255;
byte InAttractMode = false;


int RunAttractMode(int curState, boolean curStateChanged) {

  int returnState = curState;

  if (curStateChanged) {
    BSOS_DisableSolenoidStack();
    BSOS_TurnOffAllLamps();
    BSOS_SetDisableFlippers(true);
    if (DEBUG_MESSAGES) {
      Serial.write("Entering Attract Mode\n\r");
    }
    AttractLastHeadMode = 0;
    AttractLastPlayfieldMode = 0;
    BSOS_SetDisplayCredits(Credits, !FreePlayMode);
  }

  // Alternate displays between high score and blank
  if (CurrentTime < 16000) {
    if (AttractLastHeadMode != 1) {
      ShowPlayerScores(0xFF, false, false);
      BSOS_SetDisplayCredits(Credits, !FreePlayMode);
      BSOS_SetDisplayBallInPlay(0, true);
    }
  } else if ((CurrentTime / 8000) % 2 == 0) {

    if (AttractLastHeadMode != 2) {
      BSOS_SetLampState(LAMP_HEAD_HIGH_SCORE, 1, 0, 250);
      BSOS_SetLampState(LAMP_HEAD_GAME_OVER, 0);
      LastTimeScoreChanged = CurrentTime;
    }
    AttractLastHeadMode = 2;
    ShowPlayerScores(0xFF, false, false, HighScore);
  } else {
    if (AttractLastHeadMode != 3) {
      if (CurrentTime < 32000) {
        for (int count = 0; count < 4; count++) {
          CurrentScores[count] = 0;
        }
        CurrentNumPlayers = 0;
      }
      BSOS_SetLampState(LAMP_HEAD_HIGH_SCORE, 0);
      BSOS_SetLampState(LAMP_HEAD_GAME_OVER, 1);
      LastTimeScoreChanged = CurrentTime;
    }
    ShowPlayerScores(0xFF, false, false);

    AttractLastHeadMode = 3;
  }

  byte attractPlayfieldPhase = ((CurrentTime / 5000) % 5);

  if (attractPlayfieldPhase != AttractLastPlayfieldMode) {
    BSOS_TurnOffAllLamps();
    AttractLastPlayfieldMode = attractPlayfieldPhase;
    if (attractPlayfieldPhase == 2) GameMode = GAME_MODE_SKILL_SHOT_PRE_SWITCH;
    else GameMode = GAME_MODE_UNSTRUCTURED_PLAY;
    AttractLastLadderBonus = 1;
    AttractLastLadderTime = CurrentTime;
  }

  byte biggamePhase = (CurrentTime / 200) % 7;
  for (byte count = 0; count < 7; count++) {
    BSOS_SetLampState(LAMP_E + count, count == biggamePhase || count == ((biggamePhase + 4) % 7));
  }

  if (attractPlayfieldPhase == 0) {
    ShowLampAnimation(0, 30, CurrentTime, 22, false, false);
  } else if (attractPlayfieldPhase == 1) {
    ShowLampAnimation(1, 30, CurrentTime, 23, false, false);
  } else if (attractPlayfieldPhase == 3) {
    ShowLampAnimation(2, 30, CurrentTime, 23, false, false);
  } else if (attractPlayfieldPhase == 2) {
    ShowLampAnimation(0, 30, CurrentTime, 5, false, false);
  } else {
    ShowLampAnimation(2, 70, CurrentTime, 16, false, true);
  }

  byte switchHit;
  while ( (switchHit = BSOS_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
    if (switchHit == SW_CREDIT_RESET) {
      if (AddPlayer(true)) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    if (switchHit == SW_COIN_1 || switchHit == SW_COIN_2 || switchHit == SW_COIN_3) {
      AddCoinToAudit(SwitchToChuteNum(switchHit));
      AddCoin(SwitchToChuteNum(switchHit));
    }
    if (switchHit == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
      returnState = MACHINE_STATE_TEST_LIGHTS;
      SetLastSelfTestChangedTime(CurrentTime);
    }
  }

  return returnState;
}





////////////////////////////////////////////////////////////////////////////
//
//  Game Play functions
//
////////////////////////////////////////////////////////////////////////////

byte CountBits(unsigned int intToBeCounted) {
  byte numBits = 0;

  for (byte count = 0; count < 16; count++) {
    numBits += (intToBeCounted & 0x01);
    intToBeCounted = intToBeCounted >> 1;
  }

  return numBits;
}


void SetGameMode(byte newGameMode) {
  //  GameMode = newGameMode | (GameMode & ~GAME_BASE_MODE);
  GameMode = newGameMode;
  GameModeStartTime = 0;
  GameModeEndTime = 0;
  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Game mode set to %d\n", newGameMode);
    Serial.write(buf);
  }
}



boolean WaitForBallToReturn;

int InitGamePlay(boolean curStateChanged) {

  if (DEBUG_MESSAGES) {
    Serial.write("Starting game\n\r");
  }

  if (curStateChanged) {
    WaitForBallToReturn = false;
    // The start button has been hit only once to get
    // us into this mode, so we assume a 1-player game
    // at the moment
    BSOS_EnableSolenoidStack();
    BSOS_SetCoinLockout((Credits >= MaximumCredits) ? true : false);
    BSOS_TurnOffAllLamps();
    Audio.StopAllAudio();

    // Reset displays & game state variables
    for (int count = 0; count < 4; count++) {
      // Initialize game-specific variables
      SpinnerHits[count] = 0;
      CurrentScores[count] = 0;
      CardStatus[count] = 0x000000;
      CardSelected[count] = CARD_X_SELECTED;
      PreyLevel[count] = 1;
      BigGameStatus[count] = 0;
      BigGameLevel[count] = 0;
      BigGameAwarded[count] = 0;
      CardLevel[count] = 0;
      Prey[count].ResetPrey();
      LowerExtraBallCollected[count] = false;
      UpperExtraBallCollected[count] = false;
      LowerSpecialCollected[count] = false;
      UpperSpecialCollected[count] = false;
    }

    SamePlayerShootsAgain = false;
    CurrentBallInPlay = 1;
    CurrentNumPlayers = 1;
    CurrentPlayer = 0;
    ShowPlayerScores(0xFF, false, false);
    if (BSOS_ReadSingleSwitchState(SW_SAUCER)) {
      WaitForBallToReturn = true;
      BSOS_PushToSolenoidStack(SOL_SAUCER, 5);
    }
    SetGeneralIllumination(true);
  }

  if (WaitForBallToReturn) {
    if (BSOS_ReadSingleSwitchState(SW_OUTHOLE) == 0) return MACHINE_STATE_INIT_GAMEPLAY;
    WaitForBallToReturn = false;
  }

  return MACHINE_STATE_INIT_NEW_BALL;
}


int InitNewBall(bool curStateChanged, byte playerNum, int ballNum) {

  // If we're coming into this mode for the first time
  // then we have to do everything to set up the new ball
  if (curStateChanged) {
    BSOS_TurnOffAllLamps();
    SetGeneralIllumination(false);
    BallFirstSwitchHitTime = 0;
    BallSaveEndTime = 0;

    if (playerNum == 0) SkillShotLetter = BIG_GAME_LETTER_G1 << (CurrentTime % 3);
    AnimateSkillShotStartTime = 0;

    BSOS_SetDisableFlippers(false);
    BSOS_EnableSolenoidStack();
    BSOS_SetDisplayCredits(Credits, !FreePlayMode);
    if (CurrentNumPlayers > 1 && (ballNum != 1 || playerNum != 0) && !SamePlayerShootsAgain) QueueNotification(SOUND_EFFECT_VP_PLAYER_1_UP + playerNum, 1);
    SamePlayerShootsAgain = false;

    BSOS_SetDisplayBallInPlay(ballNum);
    BSOS_SetLampState(LAMP_HEAD_TILT, 0);

    CurrentBallSaveNumSeconds = BallSaveNumSeconds;
    if (BallSaveNumSeconds > 0) {
      BSOS_SetLampState(LAMP_SHOOT_AGAIN, 1, 0, 500);
    }

    BallSaveUsed = false;
    BallTimeInTrough = 0;
    NumTiltWarnings = 0;
    LastTiltWarningTime = 0;

    // Initialize game-specific start-of-ball lights & variables
    GameModeStartTime = 0;
    GameModeEndTime = 0;
    GameMode = GAME_MODE_SKILL_SHOT_PRE_SWITCH;
    SkillShotScore = 5000 * ballNum;

    // Start appropriate mode music
    if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
      BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 600);
    }

    // Reset progress unless holdover awards
    BonusXAnimationStart = 0;
    LowerExtraBall = false;
    LowerSpecial = false;
    BonusX = CalculateBonusXAndExtraBall(CardStatus[CurrentPlayer]);

    PlayfieldMultiplier = 1;
    ScoreAdditionAnimation = 0;
    ScoreAdditionAnimationStartTime = 0;
    LastSpinnerHit = 0;
    LastSaucerHitTime = CurrentTime;
    SaucerPrimed = true;
    RevealCardsStartTime = 0;
    GameLampsHitTime[0] = 0;
    GameLampsHitTime[1] = 0;
    GameLampsHitTime[2] = 0;
    GameLampsHitTime[3] = 0;
    BigGameJackpotEndTime = 0;
    Drop123HurryUp = 0;
    Drop123HurryUpPhase = 0;
    Drop456HurryUp = 0;
    Drop456HurryUpPhase = 0;
    Drop789HurryUp = 0;
    Drop789HurryUpPhase = 0;
    BIGHurryUpStart = 0;
    CardLevelIncreaseTime = 0;
    BIGGAMELevelIncreaseTime = 0;
    SnakeRolloverStart = 0;
    SnakeRolloverStatus = 0;
    SaucerSwitchStartTime = 0;

    UpdateDropsDoneFlags();

    Pop1Hits = 5;
    Pop2Hits = 0;
    Pop3Hits = 0;
    CardScan = 0;

    RightSpinnerAnimationPhase = 0;
    RightSpinnerAnimationStart = 0;

    DropTargets123.ResetDropTargets(CurrentTime + 200, true);
    DropTargets456.ResetDropTargets(CurrentTime + 400, true);
    DropTargets789.ResetDropTargets(CurrentTime + 600, true);

    Audio.StopAllAudio();
    CurrentMusicType = 0xFF;
    
  }

  // We should only consider the ball initialized when
  // the ball is no longer triggering the SW_OUTHOLE
  if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
    return MACHINE_STATE_INIT_NEW_BALL;
  } else {
    return MACHINE_STATE_NORMAL_GAMEPLAY;
  }

}


void StartBackgroundMusic(int musicType) {
  if (musicType != CurrentMusicType) {
    Audio.PlayBackgroundSoundtrack(SoundtrackUnstructured, 10, CurrentTime);
    CurrentMusicType = musicType;
  }
}


void SetBallSave(unsigned long numberOfMilliseconds) {
  BallSaveEndTime = CurrentTime + numberOfMilliseconds;
  BallSaveUsed = false;
  if (CurrentBallSaveNumSeconds == 0)  CurrentBallSaveNumSeconds = 2;
}


unsigned long LineMasks[9] = {
  0x001C0E07, // rows
  0x00E07038,
  0x070381C0,
  0x01249249, // cols
  0x02492492,
  0x04924924,
  0x04462311, // diagonals
  0x0150A854,
  0x05168B45  // corners
};

#define X_CARD_MASK 0x000001FF
#define Y_CARD_MASK 0x0003FE00
#define Z_CARD_MASK 0x07FC0000

byte CalculateBonusXAndExtraBall(unsigned long cardStatus) {
  byte linesOnX = 0;
  byte linesOnY = 0;
  byte linesOnZ = 0;
  byte nonDiagX = 0;
  byte nonDiagY = 0;
  byte nonDiagZ = 0;

  byte returnBonusX = 1;

  unsigned long maskedCard;
  unsigned long maskedLine;
  for (byte lineCount = 0; lineCount < 8; lineCount++) {
    maskedCard = cardStatus & X_CARD_MASK;
    maskedLine = LineMasks[lineCount] & X_CARD_MASK;
    if ( (maskedCard & maskedLine) == maskedLine ) {
      linesOnX += 1;
      if (lineCount < 6) nonDiagX += 1;
    }

    maskedCard = cardStatus & Y_CARD_MASK;
    maskedLine = LineMasks[lineCount] & Y_CARD_MASK;
    if ( (maskedCard & maskedLine) == maskedLine ) {
      linesOnY += 1;
      if (lineCount < 6) nonDiagY += 1;
    }

    maskedCard = cardStatus & Z_CARD_MASK;
    maskedLine = LineMasks[lineCount] & Z_CARD_MASK;
    if ( (maskedCard & maskedLine) == maskedLine ) {
      linesOnZ += 1;
      if (lineCount < 6) nonDiagZ += 1;
    }
  }

  if (linesOnZ == 8) {
    LowerExtraBall = true;
  } else {
    byte cardsWithCorners = 0;
    maskedCard = cardStatus & X_CARD_MASK;
    maskedLine = LineMasks[8] & X_CARD_MASK;
    if ( (maskedCard & maskedLine) == maskedLine ) cardsWithCorners += 1;

    maskedCard = cardStatus & Y_CARD_MASK;
    maskedLine = LineMasks[8] & Y_CARD_MASK;
    if ( (maskedCard & maskedLine) == maskedLine ) cardsWithCorners += 1;

    maskedCard = cardStatus & Z_CARD_MASK;
    maskedLine = LineMasks[8] & Z_CARD_MASK;
    if ( (maskedCard & maskedLine) == maskedLine ) cardsWithCorners += 1;

    if (cardsWithCorners > 1) {
      LowerExtraBall = true;
    } else {
      byte cardsWithTwoNonDiag = 0;
      if (nonDiagX > 1) cardsWithTwoNonDiag += 1;
      if (nonDiagY > 1) cardsWithTwoNonDiag += 1;
      if (nonDiagZ > 1) cardsWithTwoNonDiag += 1;
      if (cardsWithTwoNonDiag > 1) LowerExtraBall = true;
    }
  }

  byte numCardsWithTwoDiag = 0;
  if (linesOnX > 1 && (linesOnX - nonDiagX) == 2) numCardsWithTwoDiag += 1;
  if (linesOnY > 1 && (linesOnY - nonDiagY) == 2) numCardsWithTwoDiag += 1;
  if (linesOnZ > 1 && (linesOnZ - nonDiagZ) == 2) numCardsWithTwoDiag += 1;
  if (numCardsWithTwoDiag > 1) LowerSpecial = true;

  byte numCardsComplete = 0;
  if (linesOnX == 8) numCardsComplete += 1;
  if (linesOnY == 8) numCardsComplete += 1;
  if (linesOnZ == 8) numCardsComplete += 1;

  if (linesOnX && linesOnY && linesOnZ) {
    returnBonusX = 2;
  } else {
    if (numCardsComplete) returnBonusX = 2;
  }

  if (linesOnX > 1 && linesOnY > 1 && linesOnZ > 1) {
    returnBonusX = 3;
  } else {
    if (numCardsComplete > 1) returnBonusX = 3;
  }

  return returnBonusX;
}



boolean PositionHidden(byte posNumber) {
  if (CardStatus[CurrentPlayer] & ((unsigned long)1 << (unsigned long)posNumber)) return false;
  return true;
}


#define NUM_DROP_HURRY_UP_NOTIFICATIONS 34
unsigned int DropHurryUpNotifications[NUM_DROP_HURRY_UP_NOTIFICATIONS] {
  5000, 4750,
  4400, 4200,
  3800, 3600,
  3300, 3100,
  2800, 2650,
  2400, 2150,
  2000, 1800,
  1700, 1500,
  1400, 1250,
  1150, 1000,
  900, 800,
  700, 600,
  500, 450,
  400, 350,
  300, 250,
  200, 150,
  100, 50
};


void UpdateDropTargets() {
  DropTargets123.Update(CurrentTime);
  DropTargets456.Update(CurrentTime);
  DropTargets789.Update(CurrentTime);

  if (Drop123HurryUp) {
    if (CurrentTime < Drop123HurryUp) {
      // See if we need to play a sound or flash the lamp
      byte count;
      for (count = (NUM_DROP_HURRY_UP_NOTIFICATIONS - 1); count > 0; count--) {
        if ((Drop123HurryUp - CurrentTime) < DropHurryUpNotifications[count]) break;
      }
      if (count != Drop123HurryUpPhase) {
        if ((count % 2) == 0) {
          Audio.StopSound(SOUND_EFFECT_DROP_TARGET_HURRY_UP_1);
          PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HURRY_UP_1);
          BSOS_SetLampState(LAMP_LEFT_BANK_25K, 1);
        } else {
          BSOS_SetLampState(LAMP_LEFT_BANK_25K, 0);
        }
        Drop123HurryUpPhase = count;
      }

    } else {
      // Reset the drop bank
      Drop123HurryUp = 0;
      DropTargets123.ResetDropTargets(CurrentTime);
      BSOS_SetLampState(LAMP_LEFT_BANK_25K, 0);
    }
  }


  if (Drop456HurryUp) {
    if (CurrentTime < Drop456HurryUp) {
      // See if we need to play a sound or flash the lamp
      byte count;
      for (count = (NUM_DROP_HURRY_UP_NOTIFICATIONS - 1); count > 0; count--) {
        if ((Drop456HurryUp - CurrentTime) < DropHurryUpNotifications[count]) break;
      }
      if (count != Drop456HurryUpPhase) {
        if ((count % 2) == 0) {
          Audio.StopSound(SOUND_EFFECT_DROP_TARGET_HURRY_UP_1);
          PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HURRY_UP_1);
          BSOS_SetLampState(LAMP_TOP_RIGHT_BANK_25K, 1);
        } else {
          BSOS_SetLampState(LAMP_TOP_RIGHT_BANK_25K, 0);
        }
        Drop456HurryUpPhase = count;
      }

    } else {
      // Reset the drop bank
      Drop456HurryUp = 0;
      DropTargets456.ResetDropTargets(CurrentTime);
      BSOS_SetLampState(LAMP_TOP_RIGHT_BANK_25K, 0);
    }
  }


  if (Drop789HurryUp) {
    if (CurrentTime < Drop789HurryUp) {
      // See if we need to play a sound or flash the lamp
      byte count;
      for (count = (NUM_DROP_HURRY_UP_NOTIFICATIONS - 1); count > 0; count--) {
        if ((Drop789HurryUp - CurrentTime) < DropHurryUpNotifications[count]) break;
      }
      if (count != Drop789HurryUpPhase) {
        if ((count % 2) == 0) {
          Audio.StopSound(SOUND_EFFECT_DROP_TARGET_HURRY_UP_1);
          PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HURRY_UP_1);
          BSOS_SetLampState(LAMP_LOWER_RIGHT_BANK_25K, 1);
        } else {
          BSOS_SetLampState(LAMP_LOWER_RIGHT_BANK_25K, 0);
        }
        Drop789HurryUpPhase = count;
      }

    } else {
      // Reset the drop bank
      Drop789HurryUp = 0;
      DropTargets789.ResetDropTargets(CurrentTime);
      BSOS_SetLampState(LAMP_LOWER_RIGHT_BANK_25K, 0);
    }
  }

}

// This function manages all timers, flags, and lights
int ManageGameMode() {
  int returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

  UpdateDropTargets();

  boolean specialAnimationRunning = false;
  byte notification = 0;

  if ((CurrentTime - LastSwitchHitTime) > 3000) TimersPaused = true;
  else TimersPaused = false;

  if (RightSpinnerAnimationStart && CurrentTime > (RightSpinnerAnimationStart + 2000)) {
    RightSpinnerAnimationStart = 0;
    RightSpinnerAnimationPhase = 0;
  }

  if (CardLevelIncreaseTime && CurrentTime > (CardLevelIncreaseTime + 3000)) {
    CardLevelIncreaseTime = 0;
  }

  if (BIGGAMELevelIncreaseTime && CurrentTime > (BIGGAMELevelIncreaseTime + 3000)) {
    BIGGAMELevelIncreaseTime = 0;
  }

  if (BIGHurryUpStart != 0 && CurrentTime > (BIGHurryUpStart + BIG_HURRY_UP_DURATION)) {
    BIGHurryUpStart = 0;
  }

  if (SnakeRolloverStart && CurrentTime>(SnakeRolloverStart+1000)) {
    SnakeRolloverStart = 0;
    SnakeRolloverStatus = 0;
  }

  switch ( GameMode ) {
    case GAME_MODE_SKILL_SHOT_PRE_SWITCH:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = 0;
        LastModePrompt = 0;
        LastAlternatingScore = CurrentTime;
        SetGeneralIllumination(false);
        StartBigGameIntro(CurrentTime);
      }

      if (LastModePrompt == 0) {
        // Wait a couple of seconds before first prompt
        if ((CurrentTime - GameModeStartTime) > 5000) {
          LastModePrompt = CurrentTime;
          QueueNotification(SOUND_EFFECT_VP_PLAYER_1_UP + CurrentPlayer, 0);
        }
      } else if ((CurrentTime - LastModePrompt) > 35000) {
        LastModePrompt = CurrentTime;
        QueueNotification(SOUND_EFFECT_VP_PLAYER_1_UP + CurrentPlayer, 0);
      }

      PlayBigGameIntro(CurrentTime, &Audio);

      if (CurrentTime > (LastAlternatingScore + 2000)) {
        LastAlternatingScore = CurrentTime;
        if (((CurrentTime - GameModeStartTime) / 2000) % 2) {
          OverrideScoreDisplay(CurrentPlayer, SkillShotScore, DISPLAY_OVERRIDE_ANIMATION_FLUTTER);
          DisplaysNeedResetting = true;
        } else {
          DisplaysNeedResetting = false;
          ShowPlayerScores(0xFF, false, false);
        }
      }

      if (BallFirstSwitchHitTime != 0) {
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
        ShowPlayerScores(0xFF, false, false);
      }

      if (GameModeEndTime != 0 && CurrentTime > GameModeEndTime) {
        ShowPlayerScores(0xFF, false, false);
      }
      break;

    case GAME_MODE_SKILL_SHOT_BUILD_BONUS:
      if (GameModeStartTime == 0) {
        StopBigGameIntro(&Audio);
        GameModeStartTime = CurrentTime;
        GameModeEndTime = 0;
        LastAlternatingScore = CurrentTime;
      }

      if (CurrentTime > (LastAlternatingScore + 2000)) {
        LastAlternatingScore = CurrentTime;
        if (((CurrentTime - GameModeStartTime) / 2000) % 2) {
          OverrideScoreDisplay(CurrentPlayer, SkillShotScore, DISPLAY_OVERRIDE_ANIMATION_FLUTTER);
          DisplaysNeedResetting = true;
        } else {
          DisplaysNeedResetting = false;
          ShowPlayerScores(0xFF, false, false);
        }
      }

      if (BallFirstSwitchHitTime != 0) {
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
        ShowPlayerScores(0xFF, false, false);
      }

      if (GameModeEndTime != 0 && CurrentTime > GameModeEndTime) {
        ShowPlayerScores(0xFF, false, false);
      }
      break;


    case GAME_MODE_UNSTRUCTURED_PLAY:
      // If this is the first time in this mode
      if (GameModeStartTime == 0) {
        ShowPlayerScores(0xFF, false, false);
        StopBigGameIntro(&Audio);
        SetGeneralIllumination(true);
        GameModeStartTime = CurrentTime;
        StartBackgroundMusic(MUSIC_TYPE_UNSTRUCTURED_PLAY);
        DisplaysNeedResetting = false;
        SaucerSwitchStartTime = 0;
      }

      if (BSOS_ReadSingleSwitchState(SW_SAUCER)) {
        if (SaucerSwitchStartTime==0) {
          SaucerSwitchStartTime = CurrentTime;        
        } else if (CurrentTime>(SaucerSwitchStartTime+2000)) {
          BSOS_PushToTimedSolenoidStack(SOL_SAUCER, 5, CurrentTime + 500, true);        
        }
      } else {
        SaucerSwitchStartTime = 0;
      }

      // If all the cards are complete, they should be cycled
      if (CardLevelIncreaseTime) {
        specialAnimationRunning = true;
        ShowLampAnimation(1, 20, CurrentTime, 18, false, false);
      }

      // If BIGGAME is spelled, the level should be increased (with jackpot on saucer)


      notification = Prey[CurrentPlayer].MovePrey(CurrentTime, CardStatus[CurrentPlayer]);
      if (notification & PREY_NOTIFICATION_MOVED_COVERED || notification & PREY_NOTIFICATION_MOVED_UNCOVERED) {
        PlaySoundEffect(SOUND_EFFECT_FOOTSTEP_1 + (CurrentTime % 8));
      } else if (notification & PREY_NOTIFICATION_STOPPED_TO_REST) {
        PlaySoundEffect(SOUND_EFFECT_PANTING);
      } else if (notification & PREY_NOTIFICATION_ABOUT_TO_RUN) {
        PlaySoundEffect(SOUND_EFFECT_HEARTBEAT);
      }


      break;
    case GAME_MODE_ALL_PREY_KILLED:
      if (GameModeStartTime == 0) {
        ShowPlayerScores(0xFF, false, false);
        GameModeStartTime = CurrentTime;
        LastTimeStatusUpdated = CurrentTime;
        LastStatus = 0;
        CardScan = 0;
        CardWipe = 0;
        PreyLevel[CurrentPlayer] += 1;
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      /*
            if (PositionHidden(LastStatus)) {
              if (CurrentTime>(LastTimeStatusUpdated+150)) {
                PlaySoundEffect(SOUND_EFFECT_CLICK);
                LastTimeStatusUpdated = CurrentTime;
                CardWipe |= ((unsigned long)1<<(unsigned long)LastStatus);
                LastStatus += 1;
              }
            } else {
              if (CurrentTime>(LastTimeStatusUpdated+250)) {
                PlaySoundEffect(SOUND_EFFECT_GUN_COCK_1);
                LastTimeStatusUpdated = CurrentTime;
                CurrentScores[CurrentPlayer] += 2500;
                CardScan |= ((unsigned long)1<<(unsigned long)LastStatus);
                LastStatus += 1;
              }
            }
      */
      if (LastStatus == 27) {
        //        CardStatus[CurrentPlayer] = 0;
        //        RevealCardsStartTime = CurrentTime;
        //        IncreaseBonusX();
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      break;

    case GAME_MODE_BIG_GAME_JACKPOT_COUNTDOWN:
      if (GameModeStartTime == 0) {
        ShowPlayerScores(0xFF, false, false);
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + ((unsigned long)BigGameJackpotTime*1000);
        PlaySoundEffect(SOUND_EFFECT_BIG_GAME_LEVEL_COLLECT);
        Audio.PlayBackgroundSong(SOUND_EFFECT_EXPECTANT_MUSIC, true);
        CurrentMusicType = 0xFF;
      }

      specialAnimationRunning = true;
      ShowLampAnimation(0, 50, CurrentTime, 2, false, true);

      BigGameCollectPhase = ((CurrentTime-GameModeStartTime)/250)%7; 
      for (int count=0; count<7; count++) {
        BSOS_SetLampState(LAMP_E+count, count==BigGameCollectPhase);
      }

      for (byte count = 0; count < 4; count++) {
        int timeLeft = ((GameModeEndTime - CurrentTime) / 1000) + 1;
        if (count != CurrentPlayer) OverrideScoreDisplay(count, timeLeft, DISPLAY_OVERRIDE_ANIMATION_CENTER);
      }

      if (GameModeEndTime != 0 && CurrentTime > GameModeEndTime) {
        ShowPlayerScores(0xFF, false, false);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
        PlaySoundEffect(SOUND_EFFECT_BIG_GAME_LEVEL_COLLECT);
      }
      
      break;
      
    case GAME_MODE_BIG_GAME_JACKPOT:
      if (GameModeStartTime == 0) {
        Audio.StopAllMusic();
        ShowPlayerScores(0xFF, false, false);
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 2000;
        PlaySoundEffect(SOUND_EFFECT_JACKPOT);  
        StartScoreAnimation(100000*PlayfieldMultiplier);      
        for (int count=0; count<7; count++) {
          BSOS_SetLampState(LAMP_E+count, 1, 0, 150);
        }
      }

      specialAnimationRunning = true;
      ShowLampAnimation(2, 41, (CurrentTime-GameModeStartTime), 20, false, false);

      if (GameModeEndTime != 0 && CurrentTime > GameModeEndTime) {
        ShowPlayerScores(0xFF, false, false);
        BSOS_PushToTimedSolenoidStack(SOL_SAUCER, 5, CurrentTime + 100, true);
        PlaySoundEffect(SOUND_EFFECT_INTRO_LION_ROAR_4);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      break;

    case GAME_MODE_BIG_GAME_SAUCER_START:
      if (GameModeStartTime == 0) {
        ShowPlayerScores(0xFF, false, false);
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        SaucerPayout = false;
        // play callout where credit button cashes in
        PlaySoundEffect(SOUND_EFFECT_EXPLOSION);
      }

      specialAnimationRunning = true;
      ShowLampAnimation(3, 20, CurrentTime, 12, false, false);

      for (byte count = 0; count < 4; count++) {
        int timeLeft = ((GameModeEndTime - CurrentTime) / 1000) + 1;
        if (count != CurrentPlayer) OverrideScoreDisplay(count, timeLeft, DISPLAY_OVERRIDE_ANIMATION_CENTER);
      }

      if (SaucerPayout) {
        ShowPlayerScores(0xFF, false, false);
        SetGameMode(GAME_MODE_BIG_GAME_SAUCER_PAYOUT);
      }

      if (GameModeEndTime != 0 && CurrentTime > GameModeEndTime) {
        ShowPlayerScores(0xFF, false, false);
        SetGameMode(GAME_MODE_BIG_GAME_SAUCER_NO_PAYOUT);
      }
      break;

    case GAME_MODE_BIG_GAME_SAUCER_NO_PAYOUT:

      if (GameModeStartTime == 0) {
        ShowPlayerScores(0xFF, false, false);
        GameModeStartTime = CurrentTime;
        // Animation = 35k per level and 5k per letter
        GameModeEndTime = CurrentTime + 2000 * ((unsigned long)BigGameLevel[CurrentPlayer]) + 750 * ((unsigned long)CountBits(BigGameStatus[CurrentPlayer]));
        LastModePrompt = 0;
        BigGameLevelCollect = 0;
        BigGameLetterCollect = 0;
        BigGameCollectPhase = 0;
      }

      specialAnimationRunning = true;
      ShowLampAnimation(0, 20, CurrentTime, 12, false, false);

      // See if we need to reward a level
      if (BigGameLevelCollect < BigGameLevel[CurrentPlayer] && (LastModePrompt == 0 || CurrentTime > (LastModePrompt + 250))) {

        for (int count = 0; count < 7; count++) {
          BSOS_SetLampState(LAMP_E + count, 1, 0, 175);
        }

        if (BigGameCollectPhase < 7) {
          PlaySoundEffect(SOUND_EFFECT_BIG_GAME_LEVEL_COLLECT);
          CurrentScores[CurrentPlayer] += 5000 * PlayfieldMultiplier;
        } else {
          PlaySoundEffect(SOUND_EFFECT_INTRO_LEOPARD_GROWL_1);
        }
        BigGameCollectPhase += 1;
        LastModePrompt = CurrentTime;

        if (BigGameCollectPhase == 8) {
          BigGameCollectPhase = 0;
          BigGameLevelCollect += 1;
          if (BigGameLevelCollect == BigGameLevel[CurrentPlayer]) LastModePrompt = 0;
        }
      }

      // See if we need to reward a letter
      if (BigGameLevelCollect == BigGameLevel[CurrentPlayer] && (LastModePrompt == 0 || CurrentTime > (LastModePrompt + 750))) {
        if (BigGameLetterCollect < CountBits(BigGameStatus[CurrentPlayer])) {
          byte letterMask = 0x01;
          byte lettersSeen = 0;
          byte letterToShow = 0;

          for (int count = 0; count < 7; count++) {
            letterToShow = count;
            if (BigGameStatus[CurrentPlayer] & letterMask) lettersSeen += 1;
            if (lettersSeen == (BigGameLetterCollect + 1)) break;
            letterMask *= 2;
          }

          letterMask = 0x01;
          for (int count = 0; count < 7; count++) {
            if (count == letterToShow) BSOS_SetLampState(LAMP_E + count, 1, 0, 100);
            else BSOS_SetLampState(LAMP_E + count, (BigGameStatus[CurrentPlayer]&letterMask) ? 1 : 0);
            letterMask *= 2;
          }
          CurrentScores[CurrentPlayer] += 5000 * PlayfieldMultiplier;
          PlaySoundEffect(SOUND_EFFECT_LAUNCH);
          BigGameLetterCollect += 1;
          LastModePrompt = CurrentTime;
        } else {
          // We're finished awarding letters
        }

      }

      if (GameModeEndTime != 0 && CurrentTime > GameModeEndTime) {
        ShowPlayerScores(0xFF, false, false);
        BSOS_PushToTimedSolenoidStack(SOL_SAUCER, 5, CurrentTime + 100, true);
        PlaySoundEffect(SOUND_EFFECT_INTRO_LION_ROAR_5);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }

      break;


  }

  if ( !specialAnimationRunning && NumTiltWarnings <= MaxTiltWarnings ) {
    ShowBonusXLamps();
    ShowCardLamps();
    ShowShootAgainLamp();
    ShowBIGGAMELamps();
    ShowSaucerAndDropLamps();
    ShowLaneLamps();
  }


  // Three types of display modes are shown here:
  // 1) score animation
  // 2) fly-bys
  // 3) normal scores
  if (ScoreAdditionAnimationStartTime != 0) {
    // Score animation
    if ((CurrentTime - ScoreAdditionAnimationStartTime) < 2000) {
      byte displayPhase = (CurrentTime - ScoreAdditionAnimationStartTime) / 60;
      byte digitsToShow = 1 + displayPhase / 6;
      if (digitsToShow > 6) digitsToShow = 6;
      unsigned long scoreToShow = ScoreAdditionAnimation;
      for (byte count = 0; count < (6 - digitsToShow); count++) {
        scoreToShow = scoreToShow / 10;
      }
      if (scoreToShow == 0 || displayPhase % 2) scoreToShow = DISPLAY_OVERRIDE_BLANK_SCORE;
      byte countdownDisplay = (1 + CurrentPlayer) % 4;

      for (byte count = 0; count < 4; count++) {
        if (count == countdownDisplay) OverrideScoreDisplay(count, scoreToShow, DISPLAY_OVERRIDE_ANIMATION_NONE);
        else if (count != CurrentPlayer) OverrideScoreDisplay(count, DISPLAY_OVERRIDE_BLANK_SCORE, DISPLAY_OVERRIDE_ANIMATION_NONE);
      }
    } else {
      byte countdownDisplay = (1 + CurrentPlayer) % 4;
      unsigned long remainingScore = 0;
      if ( (CurrentTime - ScoreAdditionAnimationStartTime) < 5000 ) {
        remainingScore = (((CurrentTime - ScoreAdditionAnimationStartTime) - 2000) * ScoreAdditionAnimation) / 3000;
        if ((remainingScore / 1000) != (LastRemainingAnimatedScoreShown / 1000)) {
          LastRemainingAnimatedScoreShown = remainingScore;
          if (PlayScoreAnimationTick) PlaySoundEffect(SOUND_EFFECT_SLING_SHOT);
        }
      } else {
        CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
        remainingScore = 0;
        ScoreAdditionAnimationStartTime = 0;
        ScoreAdditionAnimation = 0;
      }

      for (byte count = 0; count < 4; count++) {
        if (count == countdownDisplay) OverrideScoreDisplay(count, ScoreAdditionAnimation - remainingScore, DISPLAY_OVERRIDE_ANIMATION_NONE);
        else if (count != CurrentPlayer) OverrideScoreDisplay(count, DISPLAY_OVERRIDE_BLANK_SCORE, DISPLAY_OVERRIDE_ANIMATION_NONE);
        else OverrideScoreDisplay(count, CurrentScores[CurrentPlayer] + remainingScore, DISPLAY_OVERRIDE_ANIMATION_NONE);
      }
    }
    if (ScoreAdditionAnimationStartTime) ShowPlayerScores(CurrentPlayer, false, false);
    else ShowPlayerScores(0xFF, false, false);
  } else {
    ShowPlayerScores(CurrentPlayer, (BallFirstSwitchHitTime == 0) ? true : false, (BallFirstSwitchHitTime > 0 && ((CurrentTime - LastTimeScoreChanged) > 2000)) ? true : false);
  }

  // Check to see if ball is in the outhole
  if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
    if (BallTimeInTrough == 0) {
      BallTimeInTrough = CurrentTime;
    } else {
      // Make sure the ball stays on the sensor for at least
      // 0.5 seconds to be sure that it's not bouncing
      if ((CurrentTime - BallTimeInTrough) > 500) {

        if (BallFirstSwitchHitTime == 0 && NumTiltWarnings <= MaxTiltWarnings) {
          // Nothing hit yet, so return the ball to the player
          BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime);
          BallTimeInTrough = 0;
          returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
        } else {
          CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
          ScoreAdditionAnimationStartTime = 0;
          ScoreAdditionAnimation = 0;
          ShowPlayerScores(0xFF, false, false);
          // if we haven't used the ball save, and we're under the time limit, then save the ball
          if (!BallSaveUsed && CurrentTime < (BallSaveEndTime + BALL_SAVE_GRACE_PERIOD)) {
            BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
            BallSaveUsed = true;
            QueueNotification(SOUND_EFFECT_VP_SHOOT_AGAIN, 8);
            BSOS_SetLampState(LAMP_SHOOT_AGAIN, 0);
            BallTimeInTrough = CurrentTime;
            returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
          } else {
            ShowPlayerScores(0xFF, false, false);
            Audio.StopAllAudio();

            if (CurrentBallInPlay < BallsPerGame) PlaySoundEffect(SOUND_EFFECT_BALL_OVER);
            returnState = MACHINE_STATE_COUNTDOWN_BONUS;
          }
        }
      }
    }
  } else {
    BallTimeInTrough = 0;
  }

  LastTimeThroughLoop = CurrentTime;
  return returnState;
}



unsigned long CountdownStartTime = 0;
unsigned long LastCountdownReportTime = 0;
unsigned long NextCountdownReportTime = 0;
unsigned long BonusCountDownEndTime = 0;
unsigned long BonusCardShown = 0;
byte BonusCountdownSpeed;

int CountdownBonus(boolean curStateChanged) {

  // If this is the first time through the countdown loop
  if (curStateChanged) {

    NextCountdownReportTime = CurrentTime;
    BonusCountDownEndTime = 0xFFFFFFFF;

    // Only count lines if we're at or above BonusLineBall
    if (CurrentBallInPlay<BonusLineBall && CardLevel[CurrentPlayer]==0) BonusCountdownProgress = 24;

    BonusCountdownProgress = 0;
    BonusCountdownSpeed = 0;
    BonusCardShown = CardStatus[CurrentPlayer];

    BSOS_TurnOffAllLamps();
    ShowBonusXLamps();
    ShowShootAgainLamp();
    if (DEBUG_MESSAGES) {
      Serial.write("Bonus countdown started\n");
    }
  }

  if (CurrentTime >= NextCountdownReportTime) {

    // If BonusCountdownProgress is less than 24, look for a line to
    // to reward
    if (BonusCountdownProgress < (24 + CardLevel[CurrentPlayer])) {
      unsigned long showLamps;
      unsigned long maskedCard;
      unsigned long maskedLine;
      boolean addToScore = false;
      if (BonusCountdownProgress < CardLevel[CurrentPlayer]) {
        showLamps = CardStatus[CurrentPlayer];
        int flashReserve = 250 / (BonusCountdownProgress + 1);
        PlaySoundEffect(SOUND_EFFECT_BIG_GAME_LEVEL_COLLECT);
        BSOS_SetLampState(LAMP_BONUS_RESERVE, 1, 0, flashReserve);
        NextCountdownReportTime = CurrentTime + 1000;
        CurrentScores[CurrentPlayer] += 27000;
      } else if (BonusCountdownProgress < (8 + CardLevel[CurrentPlayer])) {
        showLamps = CardStatus[CurrentPlayer] & ~X_CARD_MASK;
        maskedCard = CardStatus[CurrentPlayer] & X_CARD_MASK;
        maskedLine = LineMasks[BonusCountdownProgress] & X_CARD_MASK;
        if ( (maskedCard & maskedLine) == maskedLine ) {
          showLamps |= maskedLine;
          addToScore = true;
        }
      } else if (BonusCountdownProgress < (16 + CardLevel[CurrentPlayer])) {
        showLamps = CardStatus[CurrentPlayer] & ~(X_CARD_MASK | Y_CARD_MASK);
        maskedCard = CardStatus[CurrentPlayer] & Y_CARD_MASK;
        maskedLine = LineMasks[BonusCountdownProgress - 8] & Y_CARD_MASK;
        if ( (maskedCard & maskedLine) == maskedLine ) {
          showLamps |= maskedLine;
          addToScore = true;
        }
      } else {
        showLamps = 0;
        maskedCard = CardStatus[CurrentPlayer] & Z_CARD_MASK;
        maskedLine = LineMasks[BonusCountdownProgress - 16] & Z_CARD_MASK;
        if ( (maskedCard & maskedLine) == maskedLine ) {
          showLamps |= maskedLine;
          addToScore = true;
        }
      }

      if (addToScore) {
        if (NumTiltWarnings <= MaxTiltWarnings) {
          CurrentScores[CurrentPlayer] += 5000;
          PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HIT);
          NextCountdownReportTime = CurrentTime + 300;
        } else {
          NextCountdownReportTime = CurrentTime + 100;
        }
        if (DEBUG_MESSAGES) {
          Serial.write("Bonus line added\n");
        }
      }

      // Show the lamps based on calculations above
      unsigned long lampBit = 0x00000001;
      for (byte cardRow = 0; cardRow < 3; cardRow++) {
        for (byte cardCol = 0; cardCol < 3; cardCol++) {
          BSOS_SetLampState(XCardLamps[cardRow][cardCol], (showLamps & lampBit) ? true : false);
          BSOS_SetLampState(YCardLamps[cardRow][cardCol], (showLamps & (lampBit * 512)) ? true : false);
          BSOS_SetLampState(ZCardLamps[cardRow][cardCol], (showLamps & (lampBit * 512 * 512)) ? true : false);
          lampBit *= 2;
        }
      }

      BonusCountdownProgress += 1;
      if (BonusCountdownProgress==CardLevel[CurrentPlayer]) {
        if (CurrentBallInPlay < BonusLineBall) BonusCountdownProgress += 24;
      }
    } else if (BonusCountdownProgress < (105 + CardLevel[CurrentPlayer])) {
      if (BonusX < 2 && BonusCountdownProgress < (78 + CardLevel[CurrentPlayer])) BonusCountdownProgress = (78 + CardLevel[CurrentPlayer]);
      if (BonusX < 3 && BonusCountdownProgress < (51 + CardLevel[CurrentPlayer])) BonusCountdownProgress = (51 + CardLevel[CurrentPlayer]);

      unsigned long cardMask = 0x00000001;
      cardMask = cardMask << ((BonusCountdownProgress - (24 + CardLevel[CurrentPlayer])) % 27);
      BonusCardShown &= ~cardMask;

      if (CardStatus[CurrentPlayer] & cardMask) {
        if (NumTiltWarnings <= MaxTiltWarnings) CurrentScores[CurrentPlayer] += 1000;
        if (BonusCountdownSpeed == 0) {
          NextCountdownReportTime = CurrentTime + 150;
          if (NumTiltWarnings <= MaxTiltWarnings) PlaySoundEffect(SOUND_EFFECT_BUMPER_HIT_STD_2);
        } else if (BonusCountdownSpeed == 1) {
          NextCountdownReportTime = CurrentTime + 100;
          if (NumTiltWarnings <= MaxTiltWarnings) PlaySoundEffect(SOUND_EFFECT_BUMPER_HIT_STD_1);
        } else {
          NextCountdownReportTime = CurrentTime + 50;
          if (NumTiltWarnings <= MaxTiltWarnings) PlaySoundEffect(SOUND_EFFECT_BUMPER_HIT_STD_5);
        }
      }

      // Show the lamps based on calculations above
      unsigned long lampBit = 0x00000001;
      for (byte cardRow = 0; cardRow < 3; cardRow++) {
        for (byte cardCol = 0; cardCol < 3; cardCol++) {
          BSOS_SetLampState(XCardLamps[cardRow][cardCol], (BonusCardShown & lampBit) ? true : false);
          BSOS_SetLampState(YCardLamps[cardRow][cardCol], (BonusCardShown & (lampBit * 512)) ? true : false);
          BSOS_SetLampState(ZCardLamps[cardRow][cardCol], (BonusCardShown & (lampBit * 512 * 512)) ? true : false);
          lampBit *= 2;
        }
      }

      BonusCountdownProgress += 1;
      if ( ((BonusCountdownProgress - (24 + CardLevel[CurrentPlayer])) % 27) == 0 ) {
        BonusCardShown = CardStatus[CurrentPlayer];
        BonusCountdownSpeed += 1;
      }
    } else if (BonusCountdownProgress == (105 + CardLevel[CurrentPlayer])) {
      BonusCountDownEndTime = CurrentTime + 1000;
      BonusCountdownProgress += 1;
    }
  }


  if (CurrentTime > BonusCountDownEndTime) {

    // Reset any lights & variables of goals that weren't completed
    BonusCountDownEndTime = 0xFFFFFFFF;
    return MACHINE_STATE_BALL_OVER;
  }

  return MACHINE_STATE_COUNTDOWN_BONUS;
}



void CheckHighScores() {
  unsigned long highestScore = 0;
  int highScorePlayerNum = 0;
  for (int count = 0; count < CurrentNumPlayers; count++) {
    if (CurrentScores[count] > highestScore) highestScore = CurrentScores[count];
    highScorePlayerNum = count;
  }

  if (highestScore > HighScore) {
    HighScore = highestScore;
    if (HighScoreReplay) {
      AddCredit(false, 3);
      BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 3);
    }
    BSOS_WriteULToEEProm(BSOS_HIGHSCORE_EEPROM_START_BYTE, highestScore);
    BSOS_WriteULToEEProm(BSOS_TOTAL_HISCORE_BEATEN_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_HISCORE_BEATEN_START_BYTE) + 1);

    for (int count = 0; count < 4; count++) {
      if (count == highScorePlayerNum) {
        BSOS_SetDisplay(count, CurrentScores[count], true, 2);
      } else {
        BSOS_SetDisplayBlank(count, 0x00);
      }
    }

    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime + 300, true);
    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime + 600, true);
  }
}


unsigned long MatchSequenceStartTime = 0;
unsigned long MatchDelay = 150;
byte MatchDigit = 0;
byte NumMatchSpins = 0;
byte ScoreMatches = 0;

int ShowMatchSequence(boolean curStateChanged) {
  if (!MatchFeature) return MACHINE_STATE_ATTRACT;

  if (curStateChanged) {
    MatchSequenceStartTime = CurrentTime;
    MatchDelay = 1500;
    MatchDigit = CurrentTime % 10;
    NumMatchSpins = 0;
    BSOS_SetLampState(LAMP_HEAD_MATCH, 1, 0);
    BSOS_SetDisableFlippers();
    ScoreMatches = 0;
  }

  if (NumMatchSpins < 40) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      MatchDigit += 1;
      if (MatchDigit > 9) MatchDigit = 0;
      PlaySoundEffect(SOUND_EFFECT_MATCH_SPIN);
      BSOS_SetDisplayBallInPlay((int)MatchDigit * 10);
      MatchDelay += 50 + 4 * NumMatchSpins;
      NumMatchSpins += 1;
      BSOS_SetLampState(LAMP_HEAD_MATCH, NumMatchSpins % 2, 0);

      if (NumMatchSpins == 40) {
        BSOS_SetLampState(LAMP_HEAD_MATCH, 0);
        MatchDelay = CurrentTime - MatchSequenceStartTime;
      }
    }
  }

  if (NumMatchSpins >= 40 && NumMatchSpins <= 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      if ( (CurrentNumPlayers > (NumMatchSpins - 40)) && ((CurrentScores[NumMatchSpins - 40] / 10) % 10) == MatchDigit) {
        ScoreMatches |= (1 << (NumMatchSpins - 40));
        AddSpecialCredit();
        MatchDelay += 1000;
        NumMatchSpins += 1;
        BSOS_SetLampState(LAMP_HEAD_MATCH, 1);
      } else {
        NumMatchSpins += 1;
      }
      if (NumMatchSpins == 44) {
        MatchDelay += 5000;
      }
    }
  }

  if (NumMatchSpins > 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      return MACHINE_STATE_ATTRACT;
    }
  }

  for (int count = 0; count < 4; count++) {
    if ((ScoreMatches >> count) & 0x01) {
      // If this score matches, we're going to flash the last two digits
#ifdef BALLY_STERN_OS_USE_7_DIGIT_DISPLAYS
      if ( (CurrentTime / 200) % 2 ) {
        BSOS_SetDisplayBlank(count, BSOS_GetDisplayBlank(count) & 0x1F);
      } else {
        BSOS_SetDisplayBlank(count, BSOS_GetDisplayBlank(count) | 0x60);
      }
#else
      if ( (CurrentTime / 200) % 2 ) {
        BSOS_SetDisplayBlank(count, BSOS_GetDisplayBlank(count) & 0x0F);
      } else {
        BSOS_SetDisplayBlank(count, BSOS_GetDisplayBlank(count) | 0x30);
      }
#endif
    }
  }

  return MACHINE_STATE_MATCH_MODE;
}

/*
   2    2    2    1    1
   8    4    0    6    2    8    4    0
  0000 0000 1011 0011 1000 0000 0000 0111
  0000 0011 0000 0100 0010 1100 0011 1000
  0000 0100 0100 1000 0101 0011 1100 0000
*/

unsigned long DropTargetBankBitmasks[] = {
  0x00B38007,
  0x03042C38,
  0x044853C0
};

void UpdateDropsDoneFlags() {
  Drops123Done = false;
  Drops456Done = false;
  Drops789Done = false;

  if ( (CardStatus[CurrentPlayer]&DropTargetBankBitmasks[0]) == DropTargetBankBitmasks[0] ) Drops123Done = true;
  if ( (CardStatus[CurrentPlayer]&DropTargetBankBitmasks[1]) == DropTargetBankBitmasks[1] ) Drops456Done = true;
  if ( (CardStatus[CurrentPlayer]&DropTargetBankBitmasks[2]) == DropTargetBankBitmasks[2] ) Drops789Done = true;
}


void UpdateCardStatus(byte switchHit, byte overrideCardSelected = 0) {
  byte bitNumX = 0;
  byte bitNumY = 0;
  byte bitNumZ = 0;

  byte curCardSelected = CardSelected[CurrentPlayer];
  if (overrideCardSelected) curCardSelected = overrideCardSelected;

  // The X card is in order, but we have to map the targets for Y and Z
  switch (switchHit) {
    case SW_DROP_TARGET_1: bitNumX = 0; bitNumY = 7; bitNumZ = 5; break;
    case SW_DROP_TARGET_2: bitNumX = 1; bitNumY = 6; bitNumZ = 2; break;
    case SW_DROP_TARGET_3: bitNumX = 2; bitNumY = 8; bitNumZ = 3; break;
    case SW_DROP_TARGET_4: bitNumX = 3; bitNumY = 4; bitNumZ = 0; break;
    case SW_DROP_TARGET_5: bitNumX = 4; bitNumY = 1; bitNumZ = 6; break;
    case SW_DROP_TARGET_6: bitNumX = 5; bitNumY = 2; bitNumZ = 7; break;
    case SW_DROP_TARGET_7: bitNumX = 6; bitNumY = 5; bitNumZ = 1; break;
    case SW_DROP_TARGET_8: bitNumX = 7; bitNumY = 0; bitNumZ = 8; break;
    case SW_DROP_TARGET_9: bitNumX = 8; bitNumY = 3; bitNumZ = 4; break;
  }

  byte row, col;
  byte preyKilled = 0;
  unsigned long oldStatus = CardStatus[CurrentPlayer];
  if (curCardSelected & CARD_X_SELECTED) {
    CardStatus[CurrentPlayer] |= ((unsigned long)0x00000001) << bitNumX;
    // Only kill the prey if it was previously uncovered
    if (oldStatus == CardStatus[CurrentPlayer]) {
      row = (bitNumX / 3) % 3;
      col = (bitNumX) % 3;
      preyKilled += Prey[CurrentPlayer].KillPrey(0, row, col);
    }
  }
  if (curCardSelected & CARD_Y_SELECTED) {
    CardStatus[CurrentPlayer] |= ((unsigned long)0x00000200) << bitNumY;
    // Only kill the prey if it was previously uncovered
    if (oldStatus == CardStatus[CurrentPlayer]) {
      row = (bitNumY / 3) % 3;
      col = (bitNumY) % 3;
      preyKilled += Prey[CurrentPlayer].KillPrey(1, row, col);
    }
  }
  if (curCardSelected & CARD_Z_SELECTED) {
    CardStatus[CurrentPlayer] |= ((unsigned long)0x00040000) << bitNumZ;
    // Only kill the prey if it was previously uncovered
    if (oldStatus == CardStatus[CurrentPlayer]) {
      row = (bitNumZ / 3) % 3;
      col = (bitNumZ) % 3;
      preyKilled += Prey[CurrentPlayer].KillPrey(2, row, col);
    }
  }


  if (preyKilled) {
    PlaySoundEffect(SOUND_EFFECT_INTRO_MAN_SCREAM_HELP);
    StartScoreAnimation(((unsigned long)preyKilled)*PREY_KILLED_REWARD * ((unsigned long)PreyLevel[CurrentPlayer])*PlayfieldMultiplier);
    if (Prey[CurrentPlayer].GetNumPrey() == 0) {
      SetGameMode(GAME_MODE_ALL_PREY_KILLED);
    }
  }

  byte newBonusX = CalculateBonusXAndExtraBall(CardStatus[CurrentPlayer]);
  if (newBonusX != BonusX) {
    BonusX = newBonusX;
    // play a sound here? animate bonusX here?
  }

  if (CardStatus[CurrentPlayer] == 0x07FFFFFF) {
    CardStatus[CurrentPlayer] = 0;
    CardLevelIncreaseTime = CurrentTime;
    CardLevel[CurrentPlayer] += 1;
    HandlePopBumperHit(0);
    PlaySoundEffect(SOUND_EFFECT_CARD_FINISH);
  }
}


boolean StartOrShowPrey(byte switchHit) {

  if (switchHit == SW_B_ROLLOVER) BigGameStatus[CurrentPlayer] |= BIG_GAME_LETTER_B;
  if (switchHit == SW_I_ROLLOVER) BigGameStatus[CurrentPlayer] |= BIG_GAME_LETTER_I;
  if (switchHit == SW_G_ROLLOVER) BigGameStatus[CurrentPlayer] |= BIG_GAME_LETTER_G1;

  if (GameMode == GAME_MODE_SKILL_SHOT_PRE_SWITCH || GameMode == GAME_MODE_SKILL_SHOT_BUILD_BONUS) {
    boolean skillShotHit = false;
    if (switchHit == SW_B_ROLLOVER && SkillShotLetter == BIG_GAME_LETTER_B) skillShotHit = true;
    if (switchHit == SW_I_ROLLOVER && SkillShotLetter == BIG_GAME_LETTER_I) skillShotHit = true;
    if (switchHit == SW_G_ROLLOVER && SkillShotLetter == BIG_GAME_LETTER_G1) skillShotHit = true;

    if (skillShotHit) {
      StartScoreAnimation(SkillShotScore);
      PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT);
      AnimateSkillShotStartTime = CurrentTime;
    }
  }

  if (Prey[CurrentPlayer].GetNumPrey() < PreyLevel[CurrentPlayer]) {
    Prey[CurrentPlayer].InitNewPrey(CurrentTime, CurrentTime + 7000);
    PlaySoundEffect(SOUND_EFFECT_INTRO_MAN_SCREAM_FEAR);
    //    PlaySoundEffect(SOUND_EFFECT_INTRO_FAST_FOOTSTEPS);
    RevealCardsStartTime = CurrentTime;
    return true;
  } else if (RevealCardsStartTime == 0) {
    Prey[CurrentPlayer].ScatterPrey(CurrentTime, CurrentTime + 7000);
    PlaySoundEffect(SOUND_EFFECT_INTRO_TIGER_ROAR_1);
    //    PlaySoundEffect(SOUND_EFFECT_INTRO_FAST_FOOTSTEPS);
    RevealCardsStartTime = CurrentTime;
    return false;
  }


  return false;
}


void UpdateBigGameLevel() {

  if ((BigGameStatus[CurrentPlayer] & 0x70) == 0x70) {
    BIGHurryUpStart = CurrentTime;
  }

  if (BigGameStatus[CurrentPlayer] == 0x7F) {
    BigGameLevel[CurrentPlayer] += 1;
    if (BigGameLevel[CurrentPlayer] > 10) BigGameLevel[CurrentPlayer] = 10;
    BigGameStatus[CurrentPlayer] = 0;
    if (BigGameJackpotTime!=0) SetGameMode(GAME_MODE_BIG_GAME_JACKPOT_COUNTDOWN);
  }

}



void HandleSnakeRolloverHit(byte switchHit) {
  byte snakeBit = 0x01;
  if (switchHit==SW_MID_RIGHT_ROLLOVER_BUTTON) snakeBit = 0x02;
  else if (switchHit==SW_TOP_RIGHT_ROLLOVER_BUTTON) snakeBit = 0x04;

  byte oldStatus = SnakeRolloverStatus;
  if (oldStatus==0) SnakeRolloverStart = CurrentTime;
  SnakeRolloverStatus |= snakeBit;

  if (CardSelected[CurrentPlayer]&CARD_Z_SELECTED) {
    CurrentScores[CurrentPlayer] += 2000 * PlayfieldMultiplier;
  } else {
    CurrentScores[CurrentPlayer] += 100 * PlayfieldMultiplier;
  }

  if (oldStatus!=0x07 && SnakeRolloverStatus==0x07) {
    if (CardSelected[CurrentPlayer]&CARD_Z_SELECTED) {
      StartScoreAnimation(6000);
    } else {
      StartScoreAnimation(1000);
    }
  }
  PlaySoundEffect(SOUND_EFFECT_WATER_DROP_1 + CountBits(SnakeRolloverStatus) - 1);
  
}


void HandlePopBumperHit(byte switchHit) {

  byte numCardsSelected = CountBits(CardSelected[CurrentPlayer]);
  byte newCardsSelected = CardSelected[CurrentPlayer];
  int maxCards = MaxCardsLit;
  if (MaxCardsAffectedByLevel) {
    maxCards -= (int)CardLevel[CurrentPlayer];
    if (maxCards<1) maxCards = 1;
  }

  if (switchHit==SW_UPPER_POP) {
    Pop3Hits += 1;
    if (Pop3Hits == 250) Pop3Hits = 0;
    if ((Pop3Hits / 5) % 2) {
      if (maxCards==1) {
        newCardsSelected = CARD_Z_SELECTED;
      } else if (maxCards==2 && numCardsSelected==2) {
        if (!(newCardsSelected&CARD_Z_SELECTED)) newCardsSelected = CARD_Z_SELECTED | CARD_Y_SELECTED;
      } else {
        newCardsSelected |= CARD_Z_SELECTED;
      }    
    } else {
      newCardsSelected &= (~CARD_Z_SELECTED);
      if (CountBits(newCardsSelected)==0) newCardsSelected |= CARD_Z_SELECTED;
    }
  } else if (switchHit==SW_RIGHT_POP) {
    Pop2Hits += 1;
    if (Pop2Hits == 250) Pop2Hits = 0;
    if ((Pop2Hits / 5) % 2) {
      if (maxCards==1) {
        newCardsSelected = CARD_Y_SELECTED;
      } else if (maxCards==2 && numCardsSelected==2) {
        if (!(newCardsSelected&CARD_Y_SELECTED)) newCardsSelected = CARD_Y_SELECTED | CARD_X_SELECTED;
      } else {
        newCardsSelected |= CARD_Y_SELECTED;
      }
    } else {
      newCardsSelected &= (~CARD_Y_SELECTED);
      if (CountBits(newCardsSelected)==0) newCardsSelected |= CARD_Y_SELECTED;
    }
  } else {
    Pop1Hits += 1;
    if (Pop1Hits == 250) Pop1Hits = 0;
    if ((Pop1Hits / 5) % 2) {
      if (maxCards==1) {
        newCardsSelected = CARD_X_SELECTED;
      } else if (maxCards==2 && numCardsSelected==2) {
        if (!(newCardsSelected&CARD_X_SELECTED)) newCardsSelected = CARD_X_SELECTED | CARD_Z_SELECTED;
      } else {
        newCardsSelected |= CARD_X_SELECTED;
      }
    } else {
      newCardsSelected &= (~CARD_X_SELECTED);
      if (CountBits(newCardsSelected)==0) newCardsSelected |= CARD_X_SELECTED;
    }
  }

  CardSelected[CurrentPlayer] = newCardsSelected;
  if (switchHit) {
    PlaySoundEffect(SOUND_EFFECT_BUMPER_HIT_STD_3 + CurrentTime % 6);
    CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
  }
}


int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  unsigned long scoreAtTop = CurrentScores[CurrentPlayer];

  // Very first time into gameplay loop
  if (curState == MACHINE_STATE_INIT_GAMEPLAY) {
    returnState = InitGamePlay(curStateChanged);
  } else if (curState == MACHINE_STATE_INIT_NEW_BALL) {
    returnState = InitNewBall(curStateChanged, CurrentPlayer, CurrentBallInPlay);
  } else if (curState == MACHINE_STATE_NORMAL_GAMEPLAY) {
    returnState = ManageGameMode();
  } else if (curState == MACHINE_STATE_COUNTDOWN_BONUS) {
    // Reset lamps to reasonable state (in case they were in an animation)
    //    ShowBonusXLamps();
    //    ShowCardLamps();
    //    ShowShootAgainLamp();
    //    ShowBIGGAMELamps();
    //    ShowSaucerLamp();
    //    ShowLaneLamps();

    // in case GI was off, turn it on
    SetGeneralIllumination(true);
    returnState = CountdownBonus(curStateChanged);
    ShowPlayerScores(0xFF, false, false);
  } else if (curState == MACHINE_STATE_BALL_OVER) {
    BSOS_SetDisplayCredits(Credits, !FreePlayMode);

    if (SamePlayerShootsAgain) {
      QueueNotification(SOUND_EFFECT_VP_SHOOT_AGAIN, 8);
      returnState = MACHINE_STATE_INIT_NEW_BALL;
    } else {

      CurrentPlayer += 1;
      if (CurrentPlayer >= CurrentNumPlayers) {
        CurrentPlayer = 0;
        CurrentBallInPlay += 1;
      }

      scoreAtTop = CurrentScores[CurrentPlayer];

      if (CurrentBallInPlay > BallsPerGame) {
        CheckHighScores();
        PlaySoundEffect(SOUND_EFFECT_EXPLOSION);
        for (int count = 0; count < CurrentNumPlayers; count++) {
          BSOS_SetDisplay(count, CurrentScores[count], true, 2);
        }

        returnState = MACHINE_STATE_MATCH_MODE;
      }
      else returnState = MACHINE_STATE_INIT_NEW_BALL;
    }
  } else if (curState == MACHINE_STATE_MATCH_MODE) {
    returnState = ShowMatchSequence(curStateChanged);
  }

  unsigned long lastBallFirstSwitchHitTime = BallFirstSwitchHitTime;
  byte switchHit;
  byte dropTargetResult;

  if (NumTiltWarnings <= MaxTiltWarnings) {
    while ( (switchHit = BSOS_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {

      if (DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "Switch Hit = %d\n", switchHit);
        Serial.write(buf);
      }

      switch (switchHit) {
        case SW_SLAM:
          //          BSOS_DisableSolenoidStack();
          //          BSOS_SetDisableFlippers(true);
          //          BSOS_TurnOffAllLamps();
          //          BSOS_SetLampState(GAME_OVER, 1);
          //          delay(1000);
          //          return MACHINE_STATE_ATTRACT;
          break;
        case SW_TILT:
          // This should be debounced
          if ((CurrentTime - LastTiltWarningTime) > TILT_WARNING_DEBOUNCE_TIME) {
            LastTiltWarningTime = CurrentTime;
            NumTiltWarnings += 1;
            if (NumTiltWarnings > MaxTiltWarnings) {
              BSOS_DisableSolenoidStack();
              BSOS_SetDisableFlippers(true);
              BSOS_TurnOffAllLamps();
              Audio.StopAllAudio();
              PlaySoundEffect(SOUND_EFFECT_TILT);
              BSOS_SetLampState(LAMP_HEAD_TILT, 1);
              SetGeneralIllumination(false);
            }
            PlaySoundEffect(SOUND_EFFECT_TILT_WARNING);
          }
          break;
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_LIGHTS;
          SetLastSelfTestChangedTime(CurrentTime);
          break;
        case SW_CENTER_LEFT_ROLLOVER:
          if (BigGameLevel[CurrentPlayer] && BigGameAwarded[CurrentPlayer] == 0) AwardExtraBall(true);
          if (BigGameLevel[CurrentPlayer] > 1 && BigGameAwarded[CurrentPlayer] < 2) AwardSpecial(true);
          if (BigGameLevel[CurrentPlayer] > BigGameAwarded[CurrentPlayer]) {
            BigGameAwarded[CurrentPlayer] = BigGameLevel[CurrentPlayer];
          }
          CurrentScores[CurrentPlayer] += 5000 * PlayfieldMultiplier;
          PlaySoundEffect(SOUND_EFFECT_LONG_ELEPHANT);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_CENTER_ROLLOVER_BUTTON:
          CurrentScores[CurrentPlayer] += 5000 * PlayfieldMultiplier;
          PlaySoundEffect(SOUND_EFFECT_VERY_SHORT_ELEPHANT);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_TOP_LEFT_ROLLOVER_BUTTON:
          if (BIGHurryUpStart) {
            CurrentScores[CurrentPlayer] += 5000 * PlayfieldMultiplier;
            PlaySoundEffect(SOUND_EFFECT_INTRO_TIGER_ROAR_5);
          } else {
            CurrentScores[CurrentPlayer] += 500 * PlayfieldMultiplier;
            PlaySoundEffect(SOUND_EFFECT_JUNGLE_BIRD_CALL);
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_TOP_RIGHT_ROLLOVER_BUTTON:
        case SW_MID_RIGHT_ROLLOVER_BUTTON:
        case SW_LOWER_RIGHT_ROLLOVER_BUTTON:
          HandleSnakeRolloverHit(switchHit);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;          
        case SW_DROP_TARGET_1:
        case SW_DROP_TARGET_2:
        case SW_DROP_TARGET_3:
          UpdateCardStatus(switchHit);
          dropTargetResult = DropTargets123.HandleDropTargetHit(switchHit);
          if (dropTargetResult) {
            unsigned long numTargets = CountBits(dropTargetResult);
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * numTargets * 500;
            PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HIT);
            if (HurryUpDropLevel && HurryUpDropLevel>CardLevel[CurrentPlayer]) Drop123HurryUp = CurrentTime + 5000;
            Drop123HurryUpPhase = 0xFF;
            if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
            LastSwitchHitTime = CurrentTime;
          }
          dropTargetResult = DropTargets123.CheckIfBankCleared();
          if (dropTargetResult == DROP_TARGET_BANK_CLEARED) {
            PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_FINISHED);
            Drop123HurryUp = 0;
            DropTargets123.ResetDropTargets(CurrentTime + 500);
            if (HurryUpDropLevel && HurryUpDropLevel>CardLevel[CurrentPlayer] && LiberalClearRule) {
              UpdateCardStatus(SW_DROP_TARGET_1, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
              UpdateCardStatus(SW_DROP_TARGET_2, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
              UpdateCardStatus(SW_DROP_TARGET_3, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
            }
            UpdateDropsDoneFlags();
            if (Drops123Done) StartScoreAnimation(25000 * PlayfieldMultiplier);
          } else if (dropTargetResult == DROP_TARGET_BANK_CLEARED_IN_ORDER) {
            PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_FINISHED);
            Drop123HurryUp = 0;
            DropTargets123.ResetDropTargets(CurrentTime + 500);
            if (HurryUpDropLevel && HurryUpDropLevel>CardLevel[CurrentPlayer]) {
              UpdateCardStatus(SW_DROP_TARGET_1, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
              UpdateCardStatus(SW_DROP_TARGET_2, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
              UpdateCardStatus(SW_DROP_TARGET_3, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
            }
            UpdateDropsDoneFlags();
            if (Drops123Done) StartScoreAnimation(50000 * PlayfieldMultiplier);
          }
          break;
        case SW_DROP_TARGET_4:
        case SW_DROP_TARGET_5:
        case SW_DROP_TARGET_6:
          UpdateCardStatus(switchHit);
          dropTargetResult = DropTargets456.HandleDropTargetHit(switchHit);
          if (dropTargetResult) {
            unsigned long numTargets = CountBits(dropTargetResult);
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * numTargets * 500;
            PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HIT);
            if (HurryUpDropLevel && HurryUpDropLevel>CardLevel[CurrentPlayer]) Drop456HurryUp = CurrentTime + 5000;
            Drop456HurryUpPhase = 0xFF;
            if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
            LastSwitchHitTime = CurrentTime;
          }
          dropTargetResult = DropTargets456.CheckIfBankCleared();
          if (dropTargetResult == DROP_TARGET_BANK_CLEARED) {
            PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_FINISHED);
            Drop456HurryUp = 0;
            DropTargets456.ResetDropTargets(CurrentTime + 500);
            if (HurryUpDropLevel && HurryUpDropLevel>CardLevel[CurrentPlayer] && LiberalClearRule) {
              UpdateCardStatus(SW_DROP_TARGET_4, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
              UpdateCardStatus(SW_DROP_TARGET_5, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
              UpdateCardStatus(SW_DROP_TARGET_6, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
            }
            UpdateDropsDoneFlags();
            if (Drops456Done) StartScoreAnimation(25000 * PlayfieldMultiplier);
          } else if (dropTargetResult == DROP_TARGET_BANK_CLEARED_IN_ORDER) {
            PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_FINISHED);
            Drop456HurryUp = 0;
            DropTargets456.ResetDropTargets(CurrentTime + 500);
            if (HurryUpDropLevel && HurryUpDropLevel>CardLevel[CurrentPlayer]) {
              UpdateCardStatus(SW_DROP_TARGET_4, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
              UpdateCardStatus(SW_DROP_TARGET_5, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
              UpdateCardStatus(SW_DROP_TARGET_6, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
            }
            UpdateDropsDoneFlags();
            if (Drops456Done) StartScoreAnimation(50000 * PlayfieldMultiplier);
          }
          break;
        case SW_DROP_TARGET_7:
        case SW_DROP_TARGET_8:
        case SW_DROP_TARGET_9:
          UpdateCardStatus(switchHit);
          dropTargetResult = DropTargets789.HandleDropTargetHit(switchHit);
          if (dropTargetResult) {
            unsigned long numTargets = CountBits(dropTargetResult);
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * numTargets * 500;
            PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HIT);
            if (HurryUpDropLevel && HurryUpDropLevel>CardLevel[CurrentPlayer]) Drop789HurryUp = CurrentTime + 5000;
            Drop789HurryUpPhase = 0xFF;
            if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
            LastSwitchHitTime = CurrentTime;
          }
          dropTargetResult = DropTargets789.CheckIfBankCleared();
          if (dropTargetResult == DROP_TARGET_BANK_CLEARED) {
            PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_FINISHED);
            Drop789HurryUp = 0;
            DropTargets789.ResetDropTargets(CurrentTime + 500);
            if (HurryUpDropLevel && HurryUpDropLevel>CardLevel[CurrentPlayer] && LiberalClearRule) {
              UpdateCardStatus(SW_DROP_TARGET_7, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
              UpdateCardStatus(SW_DROP_TARGET_8, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
              UpdateCardStatus(SW_DROP_TARGET_9, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
            }
            UpdateDropsDoneFlags();
            if (Drops789Done) StartScoreAnimation(25000 * PlayfieldMultiplier);
          } else if (dropTargetResult == DROP_TARGET_BANK_CLEARED_IN_ORDER) {
            PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_FINISHED);
            Drop789HurryUp = 0;
            DropTargets789.ResetDropTargets(CurrentTime + 500);
            if (HurryUpDropLevel && HurryUpDropLevel>CardLevel[CurrentPlayer]) {
              UpdateCardStatus(SW_DROP_TARGET_7, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
              UpdateCardStatus(SW_DROP_TARGET_8, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
              UpdateCardStatus(SW_DROP_TARGET_9, CARD_X_SELECTED | CARD_Y_SELECTED | CARD_Z_SELECTED);
            }
            UpdateDropsDoneFlags();
            if (Drops789Done) StartScoreAnimation(50000 * PlayfieldMultiplier);
          }
          break;

        case SW_LEFT_OUTLANE:
          if (LowerSpecial) AwardSpecial(false);
          PlaySoundEffect(SOUND_EFFECT_MEDIUM_ELEPHANT);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          if (BallSaveEndTime != 0) {
            BallSaveEndTime += 3000;
          }
          break;

        case SW_RIGHT_OUTLANE:
          if (LowerExtraBall) AwardExtraBall(false);
          PlaySoundEffect(SOUND_EFFECT_MEDIUM_ELEPHANT);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          if (BallSaveEndTime != 0) {
            BallSaveEndTime += 3000;
          }
          break;

        case SW_LEFT_SLING:
        case SW_RIGHT_SLING:
          CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 10;
          PlaySoundEffect(SOUND_EFFECT_SLING_SHOT);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;

        case SW_UPPER_POP:
          HandlePopBumperHit(switchHit);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;

        case SW_RIGHT_POP:
          HandlePopBumperHit(switchHit);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;

        case SW_LEFT_POP:
          HandlePopBumperHit(switchHit);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;

        case SW_COIN_1:
        case SW_COIN_2:
        case SW_COIN_3:
          AddCoinToAudit(SwitchToChuteNum(switchHit));
          AddCoin(SwitchToChuteNum(switchHit));
          break;

        case SW_LEFT_SPINNER:
          PlaySoundEffect(SOUND_EFFECT_LEFT_SPINNER_STD);
          if (CardSelected[CurrentPlayer] & CARD_X_SELECTED) {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 200;
          } else {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 2000;
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;

        case SW_RIGHT_SPINNER:
          PlaySoundEffect(SOUND_EFFECT_RIGHT_SPINNER_STD);
          CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 100;
          RightSpinnerAnimationStart = CurrentTime;
          RightSpinnerAnimationPhase = (RightSpinnerAnimationPhase + 1) % 16;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;

        case SW_B_ROLLOVER:
          CurrentScores[CurrentPlayer] += (500 * PlayfieldMultiplier);
          StartOrShowPrey(switchHit);
          UpdateBigGameLevel();
          PlaySoundEffect(SOUND_EFFECT_TOP_LANE);
          if (GameMode == GAME_MODE_SKILL_SHOT_BUILD_BONUS || GameMode == GAME_MODE_SKILL_SHOT_PRE_SWITCH) SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;

        case SW_I_ROLLOVER:
          if (BIGHurryUpStart) {
            CurrentScores[CurrentPlayer] += 5000 * PlayfieldMultiplier;
            PlaySoundEffect(SOUND_EFFECT_INTRO_TIGER_ROAR_5);
          } else {
            CurrentScores[CurrentPlayer] += (500 * PlayfieldMultiplier);
            PlaySoundEffect(SOUND_EFFECT_TOP_LANE);
          }
          StartOrShowPrey(switchHit);
          UpdateBigGameLevel();
          if (GameMode == GAME_MODE_SKILL_SHOT_BUILD_BONUS || GameMode == GAME_MODE_SKILL_SHOT_PRE_SWITCH) SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;

        case SW_G_ROLLOVER:
          CurrentScores[CurrentPlayer] += (500 * PlayfieldMultiplier);
          StartOrShowPrey(switchHit);
          UpdateBigGameLevel();
          PlaySoundEffect(SOUND_EFFECT_TOP_LANE);
          if (GameMode == GAME_MODE_SKILL_SHOT_BUILD_BONUS || GameMode == GAME_MODE_SKILL_SHOT_PRE_SWITCH) SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;

        case SW_G_STANDUP:
          if (BigGameLevel[CurrentPlayer]) {
            CurrentScores[CurrentPlayer] += (5000 * PlayfieldMultiplier);
          } else {
            if (GameLampsHitTime[0]) CurrentScores[CurrentPlayer] += (1500 * PlayfieldMultiplier);
            else CurrentScores[CurrentPlayer] += (500 * PlayfieldMultiplier);
          }
          BigGameStatus[CurrentPlayer] |= BIG_GAME_LETTER_G2;
          UpdateBigGameLevel();
          PlaySoundEffect(SOUND_EFFECT_BUZZ_AWARD);
          GameLampsHitTime[0] = CurrentTime;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;

        case SW_A_STANDUP:
          if (BigGameLevel[CurrentPlayer]) {
            CurrentScores[CurrentPlayer] += (5000 * PlayfieldMultiplier);
          } else {
            if (GameLampsHitTime[1]) CurrentScores[CurrentPlayer] += (1500 * PlayfieldMultiplier);
            else CurrentScores[CurrentPlayer] += (500 * PlayfieldMultiplier);
          }
          BigGameStatus[CurrentPlayer] |= BIG_GAME_LETTER_A;
          UpdateBigGameLevel();
          PlaySoundEffect(SOUND_EFFECT_BUZZ_AWARD);
          GameLampsHitTime[1] = CurrentTime;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;

        case SW_LEFT_INLANE:
          if (GameLampsHitTime[2]) CurrentScores[CurrentPlayer] += (10000 * PlayfieldMultiplier);
          else if (BigGameStatus[CurrentPlayer]&BIG_GAME_LETTER_M) CurrentScores[CurrentPlayer] += (5000 * PlayfieldMultiplier);
          else CurrentScores[CurrentPlayer] += (500 * PlayfieldMultiplier);
          BigGameStatus[CurrentPlayer] |= BIG_GAME_LETTER_M;
          UpdateBigGameLevel();
          GameLampsHitTime[2] = CurrentTime;
          PlaySoundEffect(SOUND_EFFECT_SHORT_ELEPHANT);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;

        case SW_RIGHT_INLANE:
          if (GameLampsHitTime[3]) CurrentScores[CurrentPlayer] += (10000 * PlayfieldMultiplier);
          else if (BigGameStatus[CurrentPlayer]&BIG_GAME_LETTER_E) CurrentScores[CurrentPlayer] += (5000 * PlayfieldMultiplier);
          else CurrentScores[CurrentPlayer] += (500 * PlayfieldMultiplier);
          BigGameStatus[CurrentPlayer] |= BIG_GAME_LETTER_E;
          UpdateBigGameLevel();
          GameLampsHitTime[3] = CurrentTime;
          PlaySoundEffect(SOUND_EFFECT_SHORT_ELEPHANT);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;

        case SW_SAUCER:
          // Debounce the saucer switch
          if ((CurrentTime - LastSaucerHitTime) > 500) {
            if (SaucerPrimed) {
              if (GameMode==GAME_MODE_BIG_GAME_JACKPOT_COUNTDOWN) {
                SetGameMode(GAME_MODE_BIG_GAME_JACKPOT);
              } else {
                if (BigGamePayoutWithCreditButton) {
                  SetGameMode(GAME_MODE_BIG_GAME_SAUCER_START);
                } else {
                  SetGameMode(GAME_MODE_BIG_GAME_SAUCER_NO_PAYOUT);
                }
              }
              if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
              LastSwitchHitTime = CurrentTime;
              LastSaucerHitTime = CurrentTime;
              SaucerPrimed = false;
            } else {
              // Saucer not primed, kick the ball
              BSOS_PushToTimedSolenoidStack(SOL_SAUCER, 5, CurrentTime + 100, true);        
            }            
          }
          break;

        case SW_10_PT_SWITCH:
          if (GameMode == GAME_MODE_SKILL_SHOT_PRE_SWITCH || GameMode == GAME_MODE_SKILL_SHOT_BUILD_BONUS) {
            SkillShotScore += 5000 * CurrentBallInPlay;
            if (GameMode == GAME_MODE_SKILL_SHOT_PRE_SWITCH) SetGameMode(GAME_MODE_SKILL_SHOT_BUILD_BONUS);
          } else {
            CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 10;
          }
          break;

        case SW_CREDIT_RESET:
          if (GameMode == GAME_MODE_BIG_GAME_SAUCER_START) {
            SaucerPayout = true;
          } else if (MachineState == MACHINE_STATE_MATCH_MODE) {
            // If the first ball is over, pressing start again resets the game
            if (Credits >= 1 || FreePlayMode) {
              if (!FreePlayMode) {
                Credits -= 1;
                BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
                BSOS_SetDisplayCredits(Credits, !FreePlayMode);
              }
              returnState = MACHINE_STATE_INIT_GAMEPLAY;
            }
          } else {
            CreditResetPressStarted = CurrentTime;
          }
          break;
      }
    }
  } else {
    // We're tilted, so just wait for outhole
    while ( (switchHit = BSOS_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
      switch (switchHit) {
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_LIGHTS;
          SetLastSelfTestChangedTime(CurrentTime);
          break;
        case SW_COIN_1:
        case SW_COIN_2:
        case SW_COIN_3:
          AddCoinToAudit(SwitchToChuteNum(switchHit));
          AddCoin(SwitchToChuteNum(switchHit));
          break;
      }
    }
  }

  if (LastSwitchHitTime > LastSaucerHitTime) {
    SaucerPrimed = true;
  }

  if (lastBallFirstSwitchHitTime == 0 && BallFirstSwitchHitTime != 0) {
    BallSaveEndTime = BallFirstSwitchHitTime + ((unsigned long)CurrentBallSaveNumSeconds) * 1000;
  }
  if (CurrentTime > (BallSaveEndTime + BALL_SAVE_GRACE_PERIOD)) {
    BallSaveEndTime = 0;
  }

  if (!ScrollingScores && CurrentScores[CurrentPlayer] > BALLY_STERN_OS_MAX_DISPLAY_SCORE) {
    CurrentScores[CurrentPlayer] -= BALLY_STERN_OS_MAX_DISPLAY_SCORE;
  }

  if (CreditResetPressStarted) {

    if (CurrentBallInPlay < 2) {
      // If we haven't finished the first ball, we can add players
      AddPlayer();
      if (DEBUG_MESSAGES) {
        Serial.write("Start game button pressed\n\r");
      }
      CreditResetPressStarted = 0;
    } else {

      if (BSOS_ReadSingleSwitchState(SW_CREDIT_RESET)) {
        if (TimeRequiredToResetGame != 99 && (CurrentTime - CreditResetPressStarted) >= ((unsigned long)TimeRequiredToResetGame*1000)) {
          // If the first ball is over, pressing start again resets the game
          if (Credits >= 1 || FreePlayMode) {
            if (!FreePlayMode) {
              Credits -= 1;
              BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
              BSOS_SetDisplayCredits(Credits, !FreePlayMode);
            }
            returnState = MACHINE_STATE_INIT_GAMEPLAY;
            CreditResetPressStarted = 0;
          }
        }
      } else {
        CreditResetPressStarted = 0;
      }
    }

  }

  if (scoreAtTop != CurrentScores[CurrentPlayer]) {
    LastTimeScoreChanged = CurrentTime;
    if (!TournamentScoring) {
      for (int awardCount = 0; awardCount < 3; awardCount++) {
        if (AwardScores[awardCount] != 0 && scoreAtTop < AwardScores[awardCount] && CurrentScores[CurrentPlayer] >= AwardScores[awardCount]) {
          // Player has just passed an award score, so we need to award it
          if (((ScoreAwardReplay >> awardCount) & 0x01)) {
            AddSpecialCredit();
          } else if (!LowerExtraBallCollected[CurrentPlayer]) {
            AwardExtraBall(false);
          }
        }
      }
    }

  }

  return returnState;
}


void loop() {

  BSOS_DataRead(0);

  CurrentTime = millis();
  int newMachineState = MachineState;

  if (MachineState < 0) {
    newMachineState = RunSelfTest(MachineState, MachineStateChanged);
  } else if (MachineState == MACHINE_STATE_ATTRACT) {
    newMachineState = RunAttractMode(MachineState, MachineStateChanged);
  } else {
    newMachineState = RunGamePlayMode(MachineState, MachineStateChanged);
  }

  if (newMachineState != MachineState) {
    MachineState = newMachineState;
    MachineStateChanged = true;
  } else {
    MachineStateChanged = false;
  }

  BSOS_ApplyFlashToLamps(CurrentTime);
  BSOS_UpdateTimedSolenoidStack(CurrentTime);
  Audio.Update(CurrentTime);

}
