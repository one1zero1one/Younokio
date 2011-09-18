class Timer  {
  int savedTime;  //  When Timer started
  int totalTime;    //  How long Timer should last
  
  Timer (int tempTotalTime)  {
    totalTime = tempTotalTime;
  }
  
  //  Starting the timer
  void start ()  {
    savedTime = millis();  //  When the Timer starts it stores the current time in milliseconds
  }
  
  boolean isFinished ()  {
    //  Check how much time has passed
    int passedTime = millis() - savedTime;
    if (passedTime > totalTime)  {
      return true;
    }  else  {
      return false;
    }
  }
  
}

class TimedEvent extends ControlBehavior {
  long myTime;
  int interval = 1000;

  public TimedEvent() { reset(); }
  void reset() { myTime = millis() + interval; }

  public void update() {
    if(millis()>myTime) { setValue(1); reset(); }
  }
}
