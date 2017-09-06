//#include <FastLED.h>

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
	  void draw(struct CRGB leds[]);
};

Train::Train(uint16_t _position, uint16_t _tracklength)
{
	this->position = _position;
	this->NUM_LEDS = _tracklength;
	this->length = random8(10)+1;
	this->r = random8();
	this->g = random8();
	this->b = random8();
	this->speed = random8(3)+1;
 if (random8() > 128)
 {
   this->speed *= -1;
 }
}

void Train::step()
{
  int p = position + speed;
  if (p<0)
    p += NUM_LEDS;
  this->position = p % this->NUM_LEDS;
}

void Train::draw(struct CRGB leds[])
{
  if (this->speed > 0)
  {
  	for (int i = 0; i <this->length; i++)
	  {
		  int idx = (this->position + i) % this->NUM_LEDS;
	  	leds[idx] += CRGB(this->r, this->g, this->b);
  	}
  }
  else
  {
    for (int i = 0; i <this->length; i++)
    {
      uint16_t idx = (this->position + i) % this->NUM_LEDS;
      leds[idx] += CRGB(this->r, this->g, this->b);
    }
  }
}
