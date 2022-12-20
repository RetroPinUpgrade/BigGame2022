#ifndef BIG_GAME_H



boolean GeneralIlluminationOn = true;

#define LAMP_LOWER_RIGHT_BANK_25K      0
#define LAMP_TOP_RIGHT_BANK_25K        1
#define LAMP_LEFT_BANK_25K             2
#define LAMP_BONUS_EXTRA_5K            3
#define LAMP_2X                        4
#define LAMP_3X                        5
#define LAMP_BONUS_RESERVE             6
#define LAMP_X1                        7
#define LAMP_X2                        8
#define LAMP_X3                        9
#define LAMP_X4                        10
#define LAMP_X5                        11
#define LAMP_X6                        12
#define LAMP_X7                        13
#define LAMP_X8                        14
#define LAMP_X9                        15
#define LAMP_Z_AND_RIGHT_1K            16
#define LAMP_Y                         17
#define LAMP_X_AND_LEFT_SPINNER        18
#define LAMP_STANDUP_5K                19
#define LAMP_TOP_I_5K                  20
#define LAMP_SPECIAL_OUTLANE           21
#define LAMP_EXTRA_BALL_OUTLANE        22
#define LAMP_Y8                        23
#define LAMP_Y5                        24
#define LAMP_Y6                        25
#define LAMP_Y9                        26
#define LAMP_Y4                        27
#define LAMP_Y7                        28
#define LAMP_Y2                        29
#define LAMP_Y1                        30
#define LAMP_Y3                        31
#define LAMP_E                         32
#define LAMP_M                         33
#define LAMP_A                         34
#define LAMP_G2                        35
#define LAMP_G1                        36
#define LAMP_I                         37
#define LAMP_B                         38
#define LAMP_Z4                        39
#define LAMP_Z7                        40
#define LAMP_Z2                        41
#define LAMP_Z3                        42
#define LAMP_Z9                        43
#define LAMP_Z1                        44
#define LAMP_Z5                        45
#define LAMP_Z6                        46
#define LAMP_Z8                        47
#define LAMP_HEAD_HIGH_SCORE           48
#define LAMP_SHOOT_AGAIN               49
#define LAMP_HEAD_GAME_OVER            50
#define LAMP_HEAD_TILT                 51
#define LAMP_RIGHT_SPINNER_1K          53
#define LAMP_SPECIAL                   54
#define LAMP_EXTRA_BALL                55
#define LAMP_HEAD_MATCH                59


#define SW_COIN_1                   0
#define SW_COIN_2                   1
#define SW_COIN_3                   2
#define SW_LEFT_SPINNER             3
#define SW_RIGHT_SPINNER            4
#define SW_CREDIT_RESET             5
#define SW_TILT                     6
#define SW_SLAM                     7
#define SW_UPPER_POP                8
#define SW_LEFT_POP                 9
#define SW_RIGHT_POP                10
#define SW_LEFT_SLING               11
#define SW_RIGHT_SLING              12
#define SW_DROP_TARGET_3            13
#define SW_DROP_TARGET_2            14
#define SW_DROP_TARGET_1            15
#define SW_B_ROLLOVER               16
#define SW_I_ROLLOVER               17
#define SW_G_ROLLOVER               18
#define SW_G_STANDUP                19
#define SW_A_STANDUP                20
#define SW_DROP_TARGET_6            21
#define SW_DROP_TARGET_5            22
#define SW_DROP_TARGET_4            23
#define SW_CENTER_ROLLOVER_BUTTON   24
#define SW_TOP_LEFT_ROLLOVER_BUTTON 25
#define SW_CENTER_LEFT_ROLLOVER     26
#define SW_RIGHT_INLANE             27
#define SW_LEFT_INLANE              28
#define SW_DROP_TARGET_9            29
#define SW_DROP_TARGET_8            30
#define SW_DROP_TARGET_7            31
#define SW_OUTHOLE                  32
#define SW_SAUCER                   33
#define SW_RIGHT_OUTLANE            34
#define SW_LEFT_OUTLANE             35
#define SW_10_PT_SWITCH             36
#define SW_TOP_RIGHT_ROLLOVER_BUTTON    37
#define SW_MID_RIGHT_ROLLOVER_BUTTON    38
#define SW_LOWER_RIGHT_ROLLOVER_BUTTON  39

#define SOL_UPPER_POP             0
#define SOL_LEFT_POP              1
#define SOL_RIGHT_POP             2
#define SOL_LEFT_SLING            3
#define SOL_RIGHT_SLING           4
#define SOL_KNOCKER               5
#define SOL_BANK123_RESET         6
#define SOL_BANK456_RESET         7
#define SOL_BANK789_RESET         8
#define SOL_SAUCER                9
#define SOL_OUTHOLE               10

#define SOLCONT_GENERAL_ILLUMINATION  0x10

//#define SOL_NONE          15


#define NUM_SWITCHES_WITH_TRIGGERS          5
#define NUM_PRIORITY_SWITCHES_WITH_TRIGGERS 5

struct PlayfieldAndCabinetSwitch SolenoidAssociatedSwitches[] = {
  { SW_RIGHT_SLING, SOL_RIGHT_SLING, 4},
  { SW_LEFT_SLING, SOL_LEFT_SLING, 4},
  { SW_UPPER_POP, SOL_UPPER_POP, 6},
  { SW_LEFT_POP, SOL_LEFT_POP, 6},
  { SW_RIGHT_POP, SOL_RIGHT_POP, 6}
};


#define BIG_GAME_H
#endif
