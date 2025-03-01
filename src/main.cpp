#include "driver.h"

DRIVER driver;
void setup() {
  // put your setup code here, to run once:

  
  driver.init();
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.print("HELLO WORLD");
  driver.setBrightness(100);

  
  while(driver.buttonsHelding()==-1); //press any key to continue
 
  driver.returnToSystem();
}

void loop() {
  // put your main code here, to run repeatedly:
}
