//#include "BallySternOS.h"
//#include "BigGame.h"

/*
*

Pre-plunge Script
=================
00:00 - Crickets in the background X
00:10 - Thunder rolls X
00:15 - Groups of lamps in different areas flash X
00:18 - Thunder Crashes
00:23 - Lamps "rain down" from top of playfield with rain sound effect
00:40 - Thunder & lightning
00:53 - rain fades and lamps slow down a tiny bit
00:58 - "Tiger Tiger" Voice Prompt
01:05 - All lamps off, rain fades out, heartbeat
01:10 - panting and footsteps with radar animation
01:20 - growl w/ lights out
01:21 - short scream
01:25 - crickets return
02:00 - second line of poetry
02:30 - repeat from top
*
*
*
*/

#define BIG_GAME_INTRO_UNSTARTED            0
#define BIG_GAME_INTRO_CRICKETS             1
#define BIG_GAME_INTRO_THUNDER_ROLLS        2 
#define BIG_GAME_INTRO_LIGHTNING_FLASH_1    3
#define BIG_GAME_INTRO_LIGHTNING_FLASH_2    4
#define BIG_GAME_INTRO_THUNDER_CRASHES      6
#define BIG_GAME_INTRO_LAMPS_RAIN           7
#define BIG_GAME_INTRO_THUNDER_CRASHES_2    8
#define BIG_GAME_INTRO_RAIN_SLOWS           9
#define BIG_GAME_INTRO_TYGER                10
#define BIG_GAME_INTRO_DARKNESS             11
#define BIG_GAME_INTRO_PREY_CAUGHT          12
#define BIG_GAME_INTRO_CRICKET_PAUSE        13
#define BIG_GAME_INTRO_SECOND_LINE          14   
#define BIG_GAME_INTRO_REPEAT               15          
#define BIG_GAME_INTRO_DONE                 99


#define SOUND_EFFECT_INTRO_HEARTBEAT                70
#define SOUND_EFFECT_INTRO_FOOTSTEP_1               71
#define SOUND_EFFECT_INTRO_FOOTSTEP_2               72
#define SOUND_EFFECT_INTRO_FOOTSTEP_3               73
#define SOUND_EFFECT_INTRO_FOOTSTEP_4               74
#define SOUND_EFFECT_INTRO_FOOTSTEP_5               75
#define SOUND_EFFECT_INTRO_FOOTSTEP_6               76
#define SOUND_EFFECT_INTRO_FOOTSTEP_7               77
#define SOUND_EFFECT_INTRO_FOOTSTEP_8               78
#define SOUND_EFFECT_INTRO_PANTING                  79

#define SOUND_EFFECT_INTRO_CRICKETS                     300
#define SOUND_EFFECT_INTRO_ROLLING_THUNDER              301
#define SOUND_EFFECT_INTRO_SLOW_FOOTSTEPS               302
#define SOUND_EFFECT_INTRO_MEDIUM_FOOTSTEPS             303
#define SOUND_EFFECT_INTRO_FAST_FOOTSTEPS               304
#define SOUND_EFFECT_INTRO_THUNDER_CRASH                305
#define SOUND_EFFECT_INTRO_DISTANT_THUNDER              306
#define SOUND_EFFECT_INTRO_MAN_SCREAM_LONG              307
#define SOUND_EFFECT_INTRO_MAN_SCREAM_3X                308
#define SOUND_EFFECT_INTRO_MAN_SCREAM_SHORT             309
#define SOUND_EFFECT_INTRO_MAN_SCREAM_HELP              310
#define SOUND_EFFECT_INTRO_MAN_SCREAM_SHORT_ECHO        311
#define SOUND_EFFECT_INTRO_MAN_SCREAM_FEAR              312
#define SOUND_EFFECT_INTRO_MAN_SCREAM_WILHELM           313
#define SOUND_EFFECT_INTRO_GRASS_RUSTLING               314
#define SOUND_EFFECT_INTRO_TIGER_ROAR_1                 315
#define SOUND_EFFECT_INTRO_TIGER_ROAR_2                 316
#define SOUND_EFFECT_INTRO_TIGER_ROAR_3                 317
#define SOUND_EFFECT_INTRO_TIGER_ROAR_4                 318
#define SOUND_EFFECT_INTRO_TIGER_ROAR_5                 319
#define SOUND_EFFECT_INTRO_TIGER_GROWL_1                320
#define SOUND_EFFECT_INTRO_TIGER_GROWL_2                321
#define SOUND_EFFECT_INTRO_TIGER_GROWL_3                322
#define SOUND_EFFECT_INTRO_TIGER_BARK                   323
#define SOUND_EFFECT_INTRO_LION_ROAR_1                  324
#define SOUND_EFFECT_INTRO_LION_ROAR_2                  325
#define SOUND_EFFECT_INTRO_LION_ROAR_3                  326
#define SOUND_EFFECT_INTRO_LION_ROAR_4                  327
#define SOUND_EFFECT_INTRO_LION_ROAR_5                  328
#define SOUND_EFFECT_INTRO_LEOPARD_ROAR_1               329
#define SOUND_EFFECT_INTRO_LEOPARD_ROAR_2               330
#define SOUND_EFFECT_INTRO_LEOPARD_ROAR_3               331
#define SOUND_EFFECT_INTRO_LEOPARD_ROAR_4               332
#define SOUND_EFFECT_INTRO_LEOPARD_ROAR_5               333
#define SOUND_EFFECT_INTRO_LEOPARD_GROWL_1              334
#define SOUND_EFFECT_INTRO_GRASS_RUSTLING_2             335
#define SOUND_EFFECT_INTRO_GENTLE_RAIN                  336
#define SOUND_EFFECT_INTRO_TYGER_TYGER                  400
#define SOUND_EFFECT_INTRO_IN_WHAT_DISTANT              401
#define SOUND_EFFECT_INTRO_AND_WHEN_THY_HEART           402
#define SOUND_EFFECT_INTRO_WHAT_THE_HAMMER              403
#define SOUND_EFFECT_INTRO_WHAT_THE_ANVIL               404



unsigned long BigGameIntroStart = 0;
unsigned long BigGameIntroStageStep = 0;
byte BigGameIntroStage = 0;
  
byte StartBigGameIntro(unsigned long startTime) {

  BigGameIntroStart = startTime;
  BigGameIntroStage = BIG_GAME_INTRO_UNSTARTED;
  return 0;
}



byte PlayBigGameIntro(unsigned long currentTime, AudioHandler *audio) {

  unsigned long elapsedTime = currentTime - BigGameIntroStart;

  if (BigGameIntroStage==BIG_GAME_INTRO_UNSTARTED) {
    BigGameIntroStage = BIG_GAME_INTRO_CRICKETS;
  } else if (BigGameIntroStage==BIG_GAME_INTRO_CRICKETS) {
    audio->PlaySound(SOUND_EFFECT_INTRO_CRICKETS, AUDIO_PLAY_TYPE_WAV_TRIGGER);
    BigGameIntroStage = BIG_GAME_INTRO_THUNDER_ROLLS;
 
  } else if (BigGameIntroStage==BIG_GAME_INTRO_THUNDER_ROLLS && elapsedTime>10000) { 
    BSOS_TurnOffAllLamps();
    audio->FadeSound(SOUND_EFFECT_INTRO_CRICKETS, -10, 2000, false);
    audio->PlaySound(SOUND_EFFECT_INTRO_ROLLING_THUNDER, AUDIO_PLAY_TYPE_WAV_TRIGGER);
    BigGameIntroStage = BIG_GAME_INTRO_LIGHTNING_FLASH_1;
    BigGameIntroStageStep = currentTime;
  } else if (BigGameIntroStage==BIG_GAME_INTRO_LIGHTNING_FLASH_1 && elapsedTime>15000) {

    if (currentTime>(BigGameIntroStageStep+150)) {
      boolean flashOn = (currentTime%2)?true:false;

      BSOS_SetLampState(LAMP_Z_AND_RIGHT_1K, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_Y, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_X_AND_LEFT_SPINNER, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_STANDUP_5K, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_TOP_I_5K, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_E, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_M, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_A, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_G2, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_G1, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_I, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_B, flashOn, 0, 60);
      BigGameIntroStageStep = currentTime;
    }
   
    if (elapsedTime>16500) {
      BigGameIntroStage = BIG_GAME_INTRO_LIGHTNING_FLASH_2;
      boolean flashOn = false;
      BSOS_SetLampState(LAMP_Z_AND_RIGHT_1K, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_Y, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_X_AND_LEFT_SPINNER, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_STANDUP_5K, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_TOP_I_5K, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_E, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_M, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_A, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_G2, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_G1, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_I, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_B, flashOn, 0, 60);
    }
  } else if (BigGameIntroStage==BIG_GAME_INTRO_LIGHTNING_FLASH_2 && elapsedTime>17000) {
    audio->FadeSound(SOUND_EFFECT_INTRO_ROLLING_THUNDER, -20, 1000, true);
    BigGameIntroStage = BIG_GAME_INTRO_THUNDER_CRASHES;
  } else if (BigGameIntroStage==BIG_GAME_INTRO_THUNDER_CRASHES && elapsedTime>18000) {
    audio->PlaySound(SOUND_EFFECT_INTRO_THUNDER_CRASH, AUDIO_PLAY_TYPE_WAV_TRIGGER);
    BigGameIntroStage = BIG_GAME_INTRO_LAMPS_RAIN;
    BigGameIntroStageStep = 0;
  } else if (BigGameIntroStage==BIG_GAME_INTRO_LAMPS_RAIN && elapsedTime>23000) {
    if (BigGameIntroStageStep==0) {
      BSOS_TurnOffAllLamps();
      audio->PlaySound(SOUND_EFFECT_INTRO_GENTLE_RAIN, AUDIO_PLAY_TYPE_WAV_TRIGGER);
      BigGameIntroStageStep = currentTime;
    }

    ShowLampAnimation(4, 70, currentTime, 23, false, false);

    if (elapsedTime>40000) {
      BigGameIntroStage = BIG_GAME_INTRO_THUNDER_CRASHES_2;
      BigGameIntroStageStep = 0;
    }
  } else if (BigGameIntroStage==BIG_GAME_INTRO_THUNDER_CRASHES_2) {
    ShowLampAnimation(4, 70, currentTime, 23, false, false);
    if (BigGameIntroStageStep==0) {
      audio->FadeSound(SOUND_EFFECT_INTRO_CRICKETS, 10, 3000, false);
      BigGameIntroStageStep = currentTime;
    }

    if (currentTime>(BigGameIntroStageStep+125)) {
      boolean flashOn = (currentTime%2)?true:false;

      BSOS_SetLampState(LAMP_Z_AND_RIGHT_1K, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_Y, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_X_AND_LEFT_SPINNER, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_STANDUP_5K, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_TOP_I_5K, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_E, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_M, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_A, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_G2, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_G1, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_I, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_B, flashOn, 0, 60);
      BigGameIntroStageStep = currentTime;
    }

    if (elapsedTime>43000) {
      audio->PlaySound(SOUND_EFFECT_INTRO_DISTANT_THUNDER, AUDIO_PLAY_TYPE_WAV_TRIGGER);
      boolean flashOn = 0;

      BSOS_SetLampState(LAMP_Z_AND_RIGHT_1K, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_Y, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_X_AND_LEFT_SPINNER, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_STANDUP_5K, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_TOP_I_5K, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_E, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_M, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_A, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_G2, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_G1, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_I, flashOn, 0, 60);
      BSOS_SetLampState(LAMP_B, flashOn, 0, 60);
      BigGameIntroStageStep = 0;
      BigGameIntroStage = BIG_GAME_INTRO_RAIN_SLOWS;
    }
  } else if (BigGameIntroStage==BIG_GAME_INTRO_RAIN_SLOWS) {
    if (BigGameIntroStage==0) {
      BigGameIntroStageStep = currentTime;
      audio->FadeSound(SOUND_EFFECT_INTRO_CRICKETS, -20, 5000, false);
    }
    ShowLampAnimation(4, 90, currentTime, 23, false, false);

    if (elapsedTime>58000) {
      BigGameIntroStageStep = 0;
      BigGameIntroStage = BIG_GAME_INTRO_TYGER;
    }
  } else if (BigGameIntroStage==BIG_GAME_INTRO_TYGER) {
    if (BigGameIntroStageStep==0) {
      audio->PlaySound(SOUND_EFFECT_INTRO_TYGER_TYGER, AUDIO_PLAY_TYPE_WAV_TRIGGER, 3);
      BigGameIntroStageStep = currentTime;
    }
    ShowLampAnimation(4, 90, currentTime, 23, false, false);
    if (elapsedTime>65000) {
      BigGameIntroStageStep = 0;
      BSOS_TurnOffAllLamps();
      BigGameIntroStage = BIG_GAME_INTRO_DARKNESS;
    }
  } else if (BigGameIntroStage==BIG_GAME_INTRO_DARKNESS) {
    if (BigGameIntroStageStep==0) {
      BigGameIntroStageStep = 1;
      audio->PlaySound(SOUND_EFFECT_INTRO_HEARTBEAT, AUDIO_PLAY_TYPE_WAV_TRIGGER);
    }

    if (BigGameIntroStageStep==1 && elapsedTime>70000) {
      BigGameIntroStageStep = elapsedTime;
      audio->PlaySound(SOUND_EFFECT_INTRO_PANTING, AUDIO_PLAY_TYPE_WAV_TRIGGER);
    }

    if (BigGameIntroStageStep>1 && elapsedTime>(BigGameIntroStageStep+400)) {
      audio->PlaySound(SOUND_EFFECT_INTRO_FOOTSTEP_1 + (currentTime%8), AUDIO_PLAY_TYPE_WAV_TRIGGER);
      BigGameIntroStageStep = elapsedTime;
      if (elapsedTime>80000) {
        BigGameIntroStage = BIG_GAME_INTRO_PREY_CAUGHT;
        BigGameIntroStageStep = 0;
      }      
    }

    ShowLampAnimation(0, 30, currentTime, 21, false, false);

  } else if (BigGameIntroStage==BIG_GAME_INTRO_PREY_CAUGHT) {
    if (BigGameIntroStageStep==0) {
      BigGameIntroStageStep = 1;
      audio->PlaySound(SOUND_EFFECT_INTRO_TIGER_GROWL_1, AUDIO_PLAY_TYPE_WAV_TRIGGER);
      audio->FadeSound(SOUND_EFFECT_INTRO_CRICKETS, -70, 1000, true);
      BSOS_TurnOffAllLamps();
    }

    if (BigGameIntroStageStep==1 && elapsedTime>81000) {
      audio->PlaySound(SOUND_EFFECT_INTRO_MAN_SCREAM_SHORT, AUDIO_PLAY_TYPE_WAV_TRIGGER);
      BigGameIntroStage = BIG_GAME_INTRO_CRICKET_PAUSE;
      BigGameIntroStageStep = 0;
    }
  } else if (BigGameIntroStage==BIG_GAME_INTRO_CRICKET_PAUSE && elapsedTime>85000) {
    audio->PlaySound(SOUND_EFFECT_INTRO_CRICKETS, AUDIO_PLAY_TYPE_WAV_TRIGGER);
    BigGameIntroStage = BIG_GAME_INTRO_SECOND_LINE;
  } else if (BigGameIntroStage==BIG_GAME_INTRO_SECOND_LINE && elapsedTime>120000) {
    audio->PlaySound(SOUND_EFFECT_INTRO_IN_WHAT_DISTANT+currentTime%4, AUDIO_PLAY_TYPE_WAV_TRIGGER, 3);
    BigGameIntroStage = BIG_GAME_INTRO_REPEAT;
  } else if (BigGameIntroStage==BIG_GAME_INTRO_REPEAT && elapsedTime>150000) {
    BigGameIntroStage = BIG_GAME_INTRO_UNSTARTED;
    BigGameIntroStageStep = 0;
    BigGameIntroStart = currentTime;
  }


  return 1;
}


void StopBigGameIntro(AudioHandler *audio) {
  audio->StopSound(SOUND_EFFECT_INTRO_CRICKETS);
  audio->StopSound(SOUND_EFFECT_INTRO_ROLLING_THUNDER);
}
