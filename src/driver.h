
#ifndef driver_h
#define driver_h
#define DISABLE_ALL_LIBRARY_WARNINGS
#include <Arduino.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <MCP23017.h>
#include <esp_ota_ops.h>
#include <Wire.h>
#define BACK 0
#define SELECT 1
#define UP 2
#define DOWN 3
#define LEFT 4
#define RIGHT 5
#define ANSWER 6
#define DECLINE BACK
#define MCP23017_ADDR 0x20
#define MAX_BRIGHTNESS 255
#define SD_CHIP_SELECT 13


//MCP23017 keyboard object
extern MCP23017 mcp;
//TFT screen object
extern TFT_eSPI tft;

/*
 * RGB 565 struct for drawFromSd commands
 * addr - address in file
 * trans_color - color that will not render 
 */
struct SDImage
{
    uint32_t address;
    int w;
    int h;
    uint16_t tc;
    bool transp;
    SDImage(uint32_t addr, int width, int height, uint16_t trans_color, bool transparency)
        : address(addr), w(width), h(height), tc(trans_color), transp(transparency) {}
    SDImage() : address(0), w(0), h(0), tc(0), transp(false) {}
    SDImage(uint32_t addr, int width, int height)
        : address(addr), w(width), h(height), tc(0), transp(false) {}
};

//class of driver library
class DRIVER
{
public:
    //should be called at the beginning
    //inits hardware
    void init();
    //inits SDCard at different speeds
    bool initSDCard(bool fast);
    // Sets cpu and sdcard to slow or fast frequencies
    void fastMode(bool status);
    //Throw full screen error
    void sysError(const char *reason);
    
    //check what button is pressed
    int buttonsHelding();
    void setBrightness(int percentage);
    int getBrightness();

    void drawFromSd(uint32_t pos, int pos_x, int pos_y, int size_x, int size_y, String file_path, bool transp, uint16_t tc);
    void drawFromSd(uint32_t pos, int pos_x, int pos_y, int size_x, int size_y, bool transp, uint16_t tc, String file_path);
    void drawFromSd(int x, int y, SDImage sprite, String file_path);
    void returnToSystem();
private:
    int checkEXbutton();
};



#endif