#include <FastLED.h>

class Train
{
	private:
	  uint8_t length;
	  uint8_t r;
	  uint8_t g;
	  uint8_t b;
	  int8_t speed;	  
	  uint16_t position;
	  uint16_t NUM_LEDS;	  
	public:
	  Train(uint16_t _position, uint16_t _tracklength);
	  void step();	  
	  void draw(CRGBArray leds);
}

class Track
{
	private:
	  uint16_t NUM_LEDS;	  
	  uint8_t NUM_TRAINS;
	  Train[10] *trains;
	public:
	  Track(uint16_t _tracklength);
	  void step();
	  void draw(CRGBArray leds);
}

Train::Train(uint16_t _position)
{
	this->position = _position;
	this->NUM_LEDS = _tracklength;
	this->length = random8(10)+3;
	this->r = random8();
	this->g = random8();
	this->b = random8();
	this->speed = random8(2)+1;
}

void Train::step()
{
	this->position += this->speed;
	this->position %= this->NUM_LEDS;	
}

void Train::draw(CRGBArray leds)
{
	for (int i = 0; i <this->length; i++)
	{
		int idx = (this->position + i) % this->NUM_LEDS;
		leds[idx] += CRGB(this->r, this->g, this->b);
	}
}


Track::Track(uint16_t _tracklength);
{
	this->NUM_LEDS =_tracklength;
	this->NUM_TRAINS = _num_trains;
	for (int i=0; i<10;i++)
	{
		trains[i] = new Train(_tracklength);
	}
}

void Track::step()
{
	for (int i=0; i<10;i++)
	{
		trains[i]->step();
	}
}

void Track::draw(CRGBArray leds)
{
	// clear leds
	leds(0, this->NUM_LEDS, CRGB::Black);
	
	// draw each train
	for (int i=0; i<10;i++)
	{
		trains[i]->draw(leds);
	}
}
