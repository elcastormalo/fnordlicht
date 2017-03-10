//#include <FastLED.h>
//#include "trains.h"


class Track
{
  private:
    uint16_t NUM_LEDS;    
    uint8_t NUM_TRAINS;
    Train *trains[15];
  public:
    Track(unsigned int _tracklength);
    void step();
    void draw(struct CRGB leds[]);
};

Track::Track(unsigned int _tracklength)
{
  this->NUM_LEDS =_tracklength;
  this->NUM_TRAINS = 15;
  for (int i=0; i<NUM_TRAINS;i++)
  {
    trains[i] = new Train( random16(NUM_LEDS),_tracklength);
  }
}

void Track::step()
{
  for (int i=0; i<NUM_TRAINS;i++)
  {
    trains[i]->step();
  }
}

void Track::draw(struct CRGB leds[])
{
  // clear leds
  for(int i=0; i< this->NUM_LEDS;i++)
  {
    leds[i] = CRGB::Black;
  }
  //leds(0, this->NUM_LEDS, CRGB::Black);
  
  // draw each train
  for (int i=0; i<NUM_TRAINS;i++)
  {
    trains[i]->draw(leds);
  }
}
