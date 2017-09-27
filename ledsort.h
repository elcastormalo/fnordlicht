class LEDSort
{
  private:
    const uint16_t NUM_LEDS = 600;
    const uint8_t ITEM_SIZE = 5;
	const uint8_t NUM_ITEMS = NUM_LEDS / ITEM_SIZE;
	byte hues[NUM_ITEMS];
  public:
    LEDSort();
    virtual void step() = 0;
    void draw(struct CRGB leds[]);
	void init();
	void swap(int left, int right);
	void compare(int left, int right);
};

void LEDSort::draw(struct CRGB leds[])
{
  for (int i = 0; i < NUM_ITEMS; i++)
  {
    byte h = sort_array[i];
    for (int j = 0; j < ITEM_SIZE; j++)
    {
      leds[i*ITEM_SIZE + j] = CHSV(h, 255, 200);
    }
  }  
}

void LEDSort::init()
{
  //sorted = false;
  for (int i = 0; i < NUM_ITEMS; i++)
  {
    this->hues[i] = random8();
  }
}

void LEDSort::swap(int left, int right)
{
  left %= 120;
  right %= 120;
  byte h = this->hues[left];
  this->hues[left] = this->hues[right];
  this->hues[right] = h;
  //displaysort();  
}

void LEDSort::compare(int left, int right)
{
  left %= 120;
  right %= 120;
  return this->hues[left] < this->hues[right];
}

