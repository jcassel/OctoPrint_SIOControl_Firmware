class TimeRelease
{
public:
    TimeRelease(void);
    void set_max_delay(unsigned long v);
    void set(void);
    void clear(void);
    void set(unsigned long v);
    boolean check(void);
    boolean isRunning(void);
    unsigned long timeLeft(void);
    
private:
    unsigned long max_delay;
    unsigned long last_set;
    bool running = false;
    unsigned long MSLeft;
};

TimeRelease::TimeRelease(void)
{
    max_delay = 30000UL; // default 30 seconds
}

void TimeRelease::set_max_delay(unsigned long v)
{
    max_delay = v;
    set();
}

void TimeRelease::set(unsigned long v){
  set_max_delay(v);
  set();
}

void TimeRelease::set()
{
    last_set = millis();
    running = true;
}

void TimeRelease::clear()
{
    last_set = 0;
    running = false;
}

boolean TimeRelease::isRunning(){
  return running;
}

unsigned long TimeRelease::timeLeft(){
  if(running){
    return (max_delay - MSLeft) / 1000;
  }else{
    return 0;
  }
}


boolean TimeRelease::check()
{
    if(running == false){return false;} //if the timer is not running it cant have run out. Allows for run out to trigger only once.
    
    unsigned long now = millis();
    MSLeft = now-last_set;
    if (now - last_set > max_delay) {
        last_set = now;
        running = false;
        return true;
    }
    
    return false;
}
