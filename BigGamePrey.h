#define MAX_NUMBER_OF_PREY  4

#define PREY_STATE_UNINITIALIZED          0
#define PREY_STATE_RUNNING                1
#define PREY_STATE_RESTING_COVERED        2
#define PREY_STATE_RESTING_UNCOVERED      3
#define PREY_STATE_RESTED                 4

#define PREY_NOTIFICATION_NO_CHANGE         0
#define PREY_NOTIFICATION_MOVED_UNCOVERED   1
#define PREY_NOTIFICATION_MOVED_COVERED     2
#define PREY_NOTIFICATION_STOPPED_TO_REST   4
#define PREY_NOTIFICATION_ABOUT_TO_RUN      8

#define PREY_RUN_TIME                     12000
#define PREY_REST_TIME                    30000
#define PREY_WAIT_BEFORE_BOLT_TIME        10000


struct PreyDestinations {
  byte targetCell[4];
};

PreyDestinations CellDestinations[27] = {
  {0xFF, 1, 3, 0xFF},
  {0xFF, 2, 4, 1},
  {0xFF, 9, 5, 1},
  {0, 4, 6, 0xFF},
  {1, 5, 7, 3},
  {2, 12, 8, 4},
  {3, 7, 0xFF, 0xFF},
  {4, 8, 0xFF, 6},
  {5, 15, 18, 7},

  {0xFF, 10, 12, 2},
  {0xFF, 11, 13, 9},
  {0xFF, 0xFF, 14, 10},
  {9, 13, 15, 5},
  {10, 14, 16, 12},
  {11, 0xFF, 17, 13},
  {12, 16, 20, 8},
  {13, 17, 0xFF, 15},
  {14, 0xFF, 0xFF, 16},

  {8, 19, 21, 0xFF},
  {0xFF, 20, 22, 18},
  {15, 0xFF, 23, 19},
  {18, 22, 24, 0xFF},
  {19, 23, 25, 21},
  {20, 0xFF, 26, 22},
  {21, 25, 0xFF, 0xFF},
  {22, 26, 0xFF, 24},
  {23, 0xFF, 0xFF, 25}
  
};

class BigGamePrey
{
  public:
    BigGamePrey();
    ~BigGamePrey() {}

    byte InitNewPrey(unsigned long startTime, unsigned long endTime);
    byte ScatterPrey(unsigned long startTime, unsigned long endTime);
    byte GetNumPrey();

    byte MovePrey(unsigned long curTime, unsigned long cardStatus);
    byte CheckPosition(byte card, byte row, byte col);
    byte KillPrey(byte card, byte row, byte col);

    void ResetPrey();

  private:
    byte mPreyCell[MAX_NUMBER_OF_PREY];
    byte mPreyDir[MAX_NUMBER_OF_PREY];
    byte mPreyState[MAX_NUMBER_OF_PREY];
    unsigned long mPreyStateStartTime[MAX_NUMBER_OF_PREY];
    unsigned long mPreyStateEndTime[MAX_NUMBER_OF_PREY];
    unsigned long mLastPreyMoveTime[MAX_NUMBER_OF_PREY];

    boolean LocationHidden(byte cell, unsigned long cardStatus);
};

void BigGamePrey::ResetPrey() {
  for (byte count=0; count<MAX_NUMBER_OF_PREY; count++) {
    mPreyStateStartTime[count] = 0;
    mPreyStateEndTime[count] = 0;
    mPreyCell[count] = 0xFF;
    mPreyDir[count] = 0;
    mPreyState[count] = PREY_STATE_UNINITIALIZED;
  }
}

BigGamePrey::BigGamePrey() {
  ResetPrey();
}

byte BigGamePrey::CheckPosition(byte card, byte row, byte col) {
  byte checkCell = row*3 + col + card*9;
  byte numPrey = 0;
  for (byte count=0; count<MAX_NUMBER_OF_PREY; count++) {
    if (mPreyCell[count]==checkCell && mPreyState[count]!=PREY_STATE_UNINITIALIZED) numPrey += 1;
  }  
  return numPrey;
}

byte BigGamePrey::InitNewPrey(unsigned long startTime, unsigned long endTime) {
  for (byte count=0; count<MAX_NUMBER_OF_PREY; count++) {
    if (mPreyState[count]==PREY_STATE_UNINITIALIZED) {
      unsigned long curMicros = millis();
      mPreyCell[count] = curMicros%27;
      mPreyStateStartTime[count] = startTime;
      mPreyStateEndTime[count] = endTime;
      mLastPreyMoveTime[count] = startTime;
      mPreyDir[count] = (curMicros/27)%4;
      mPreyState[count] = PREY_STATE_RUNNING;
      return true;
    }
  }
  return false;  
}

byte BigGamePrey::ScatterPrey(unsigned long startTime, unsigned long endTime) {
  byte numPrey = 0;
  for (byte count=0; count<MAX_NUMBER_OF_PREY; count++) {
    if (mPreyState[count]!=PREY_STATE_UNINITIALIZED) {
      mPreyStateStartTime[count] = startTime;
      mPreyStateEndTime[count] = endTime;
      mLastPreyMoveTime[count] = startTime;
      mPreyState[count] = PREY_STATE_RUNNING;
      numPrey += 1;
    }
  }

  return numPrey;
}


byte BigGamePrey::GetNumPrey() {
  byte numPrey = 0;

  for (byte count=0; count<MAX_NUMBER_OF_PREY; count++) {
    if (mPreyState[count]!=PREY_STATE_UNINITIALIZED) numPrey += 1;
  }

  return numPrey;
}


boolean BigGamePrey::LocationHidden(byte cell, unsigned long cardStatus) {

  unsigned long cardBitMask = ((unsigned long)0x00000001) << ((unsigned long)cell);
  if (cardStatus&cardBitMask) return false;

  return true;
}

boolean RestedLocationReported = true;

byte BigGamePrey::MovePrey(unsigned long curTime, unsigned long cardStatus) {

  unsigned long curSeed = millis();
  byte numPreyMoving = 0;
  byte preyNotification = PREY_NOTIFICATION_NO_CHANGE;

  for (byte count=0; count<MAX_NUMBER_OF_PREY; count++) {

    if (mPreyState[count]==PREY_STATE_UNINITIALIZED) continue;
    
    if (mPreyStateStartTime[count]!=0) {
      if (curTime>mPreyStateEndTime[count]) {
        if (mPreyState[count]==PREY_STATE_RUNNING) {
          if (LocationHidden(mPreyCell[count], cardStatus)) {
            mPreyState[count] = PREY_STATE_RESTING_COVERED;
            mPreyStateStartTime[count] = curTime;
            mPreyStateEndTime[count] = curTime + PREY_REST_TIME;
          } else {
            mPreyState[count] = PREY_STATE_RESTING_UNCOVERED;
            mPreyStateStartTime[count] = curTime;
            mPreyStateEndTime[count] = curTime + PREY_REST_TIME;
            mLastPreyMoveTime[count] = curTime;
          }
        } else if (mPreyState[count]==PREY_STATE_RESTING_COVERED) {
          mPreyState[count] = PREY_STATE_RESTED;
          mPreyStateStartTime[count] = 0;
          mPreyStateEndTime[count] = 0;
          RestedLocationReported = false;
        } else if (mPreyState[count]==PREY_STATE_RESTING_UNCOVERED) {
          mPreyState[count] = PREY_STATE_RUNNING;
          mPreyStateStartTime[count] = curTime;
          mPreyStateEndTime[count] = curTime + PREY_RUN_TIME;
          mLastPreyMoveTime[count] = curTime;
        } else if (mPreyState[count]==PREY_STATE_RESTED) {
          mPreyState[count] = PREY_STATE_RUNNING;
          mPreyStateStartTime[count] = curTime;
          mPreyStateEndTime[count] = curTime + PREY_RUN_TIME;
          mLastPreyMoveTime[count] = curTime;
        }
      } else if (mPreyState[count]==PREY_STATE_RUNNING) {
        if (curTime>(mLastPreyMoveTime[count]+250)) {
          mLastPreyMoveTime[count] = curTime;
          byte curCell = mPreyCell[count];

          if ( (curCell==8 || curCell==15) && mPreyDir[count]!=0 ) {
            mPreyDir[count] = 2;
          } else if ( (curSeed%10)==0 ) {
            // We're going to randomly change direction 1/10th of the time
            curSeed = millis();
            mPreyDir[count] = curSeed%4;                        
          }

          byte newCell = CellDestinations[curCell].targetCell[mPreyDir[count]];

          if (newCell<27) {
            if (newCell!=mPreyCell[count]) {
              if (LocationHidden(mPreyCell[count], cardStatus)) preyNotification |= PREY_NOTIFICATION_MOVED_COVERED;
              else preyNotification |= PREY_NOTIFICATION_MOVED_UNCOVERED;
            }
            mPreyCell[count] = newCell;
          }
          else mPreyDir[count] = (mPreyDir[count] + 1)%4;
          numPreyMoving += 1;
        }
      } else if (mPreyState[count]==PREY_STATE_RESTING_COVERED) {
        if (!LocationHidden(mPreyCell[count], cardStatus)) {
          mPreyState[count] = PREY_STATE_RESTING_UNCOVERED;
        }
      } else if (mPreyState[count]==PREY_STATE_RESTING_UNCOVERED) {
        if (curTime>(mPreyStateStartTime[count]+5000) && mLastPreyMoveTime[count]==mPreyStateStartTime[count]) {
          mLastPreyMoveTime[count] = curTime;
          preyNotification |= PREY_NOTIFICATION_STOPPED_TO_REST;      
        } else if (curTime>(mPreyStateEndTime[count]-5000) && mLastPreyMoveTime[count]<(mPreyStateEndTime[count]-5000)) {
          mLastPreyMoveTime[count] = curTime;
          preyNotification |= PREY_NOTIFICATION_ABOUT_TO_RUN;      
        }
      } 
    } else if (mPreyState[count]==PREY_STATE_RESTED) {
      if (!RestedLocationReported) {
        RestedLocationReported = true;
      }
      if (mPreyStateStartTime[count]==0) {
        if (!LocationHidden(mPreyCell[count], cardStatus)) {
          mPreyStateStartTime[count] = curTime;
          mPreyStateEndTime[count] = curTime + PREY_WAIT_BEFORE_BOLT_TIME;
          mLastPreyMoveTime[count] = curTime;
        }
      } else if (curTime>(mPreyStateEndTime[count]-5000) && mLastPreyMoveTime[count]<(mPreyStateEndTime[count]-5000)) {
        mLastPreyMoveTime[count] = curTime;
        preyNotification |= PREY_NOTIFICATION_ABOUT_TO_RUN;      
      }
    }

    curSeed /= 8;
  }

  return preyNotification;  
}


byte BigGamePrey::KillPrey(byte card, byte row, byte col) {

  int numPrey = 0;
  byte checkCell = row*3 + col + card*9;

  if (checkCell>=27) return 0;

  for (byte count=0; count<MAX_NUMBER_OF_PREY; count++) {
    if (mPreyState[count]!=PREY_STATE_UNINITIALIZED && mPreyCell[count]==checkCell) {
      numPrey += 1;
      mPreyState[count] = PREY_STATE_UNINITIALIZED;
      mPreyStateStartTime[count] = 0;
      mPreyStateEndTime[count] = 0;    
    }
  }

  return numPrey;
}
