#include "driver.h"

MCP23017 mcp = MCP23017(MCP23017_ADDR);
TFT_eSPI tft = TFT_eSPI();

void DRIVER::init()
{

    pinMode(TFT_BL, OUTPUT);
    analogWrite(TFT_BL, 0);

    Wire.begin(21, 22);
    mcp.init();
    mcp.writeRegister(MCP23017Register::GPIO_A, 0x00); // Reset port A
    mcp.writeRegister(MCP23017Register::GPIO_B, 0x00); // Reset port B
    tft.init();
    esp_ota_set_boot_partition(esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "app0"));
}

bool DRIVER::initSDCard(bool fast)
{
    SD.end();
    ulong sd_freq = 80000000;

    if (fast)
    {
        while (sd_freq >= 1000000)
        {
            if (SD.begin(SD_CHIP_SELECT, SPI, sd_freq))
            {
                return true;
            }
            sd_freq -= 1000000;
        }
    }

    return SD.begin(SD_CHIP_SELECT, SPI, 500000);
}

void DRIVER::fastMode(bool status)
{
    setCpuFrequencyMhz(status ? 240 : 20);
    initSDCard(status);
}
void DRIVER::sysError(const char *reason)
{
    tft.fillScreen(0x0000);
    tft.setCursor(10, 40);
    tft.setTextFont(1);
    tft.setTextSize(4);
    tft.setTextColor(0xF001); // FOOL :3
    tft.println("==ERROR==");
    tft.setTextColor(0xFFFF);
    tft.setTextSize(1);
    tft.println(String("\n\n\nThere a problem with your device\nYou can fix it by yourself i guess\nThere some details for you:\n\n\nReason:" + String(reason)));
    for (;;)
        ;
}
int DRIVER::checkEXbutton()
{

    mcp.portMode(MCP23017Port::A, 0xFF);
    mcp.portMode(MCP23017Port::B, 0);
    mcp.writePort(MCP23017Port::B, 0x00);

    uint8_t a = mcp.readPort(MCP23017Port::A);

    mcp.portMode(MCP23017Port::B, 0xFF);
    mcp.portMode(MCP23017Port::A, 0);
    mcp.writePort(MCP23017Port::A, 0x00);
    uint8_t b = mcp.readPort(MCP23017Port::B);

    a = ~a;
    a >>= 1;
    b = ~b;

    uint8_t ar = 0xFF, br = 0xFF;
    for (int i = 0; i < 8; ++i)
    {
        if (a & (1 << i) && ar == 0xFF)
            ar = i;

        if (b & (1 << i) && br == 0xFF)
            br = i;
    }
    ar++;
    br++;

    if (ar != 0xFF && br != 0xFF)
    {
        uint8_t res = ar == 0 && br == 0 ? 0 : 21 - (ar * 3) + br;

        return res;
    }
    return 0;
}

int DRIVER::buttonsHelding()
{
    int result = checkEXbutton();
    if (result != 0)
        while (result == checkEXbutton())
            ;
    else
        return -1;

    switch (result)
    {
    case 2:
        return UP;
    case 4:
        return LEFT;
    case 5:
        return SELECT;
    case 6:
        return RIGHT;
    case 7:
        return ANSWER;
    case 8:
        return DOWN;
    case 9:
        return DECLINE;
    case 19:
        return '*';

    case 20:
        return '0';

    case 21:
        return '#';

    default:
        if (result > 9 && result < 19)
        {
            result -= 10;
            result += '1';
            return result;
        }
        break;
    }

    if (Serial.available())
    {
        char input = Serial.read();
        switch (input)
        {
        case 'a':
            Serial.println("LEFT");
            return LEFT;
            break;
        case 'd':
            Serial.println("RIGHT");
            return RIGHT;
            break;
        case 'w':
            Serial.println("UP");
            return UP;
            break;
        case 's':
            Serial.println("DOWN");
            return DOWN;
            break;
        case ' ':
            Serial.println("SELECT");
            return SELECT;
            break;
        case 'e':
            Serial.println("ANSWER");
            return ANSWER;
            break;
        case 'q':
            Serial.println("BACK");
            return BACK;
            break;
        case '*':
            Serial.println(input);
            return input;
            break;
        case '#':
            Serial.println(input);
            return input;
            break;
        default:
            if (input >= '0' && input <= '9')
            {
                Serial.println(input);
                return input;
            }
            break;
        }
    }
    return -1;
}

int currentBrightness = 0;
void DRIVER::setBrightness(int percentage)
{
    percentage = constrain(percentage, 0, 100);

    if (percentage > currentBrightness)
    {
        for (int i = currentBrightness; i <= percentage; i++)
        {
            analogWrite(TFT_BL, (MAX_BRIGHTNESS * i) / 100);
            delay(5);
        }
    }
    else
    {
        for (int i = currentBrightness; i > percentage; i--)
        {
            analogWrite(TFT_BL, (MAX_BRIGHTNESS * i) / 100);
            delay(5);
        }
    }
    currentBrightness = percentage;
}
int DRIVER::getBrightness()
{
    return currentBrightness;
}

void DRIVER::drawFromSd(uint32_t pos, int pos_x, int pos_y, int size_x, int size_y, String file_path, bool transp, uint16_t tc)
{
    fastMode(true);
    File file = SD.open(file_path, FILE_READ);
    if (!file.available())
        sysError("FILE_NOT_AVAILABLE");
    file.seek(pos);
    if (!transp)
    {
        const int buffer_size = size_x * 2;
        uint8_t buffer[buffer_size];

        for (int a = 0; a < size_y; a++)
        {
            file.read(buffer, buffer_size);

            tft.pushImage(pos_x, a + pos_y, size_x, 1, (uint16_t *)buffer);
        }
    }
    else
    {
        const int buffer_size = size_x * 2; // 2 bytes per pixel
        uint8_t buffer[buffer_size];

        for (int a = 0; a < size_y; a++)
        {
            // Read a whole line (row) of pixels at once
            file.read(buffer, buffer_size);

            int draw_start = -1; // Initialize start of draw segment

            for (int i = 0; i < size_x; i++)
            {
                // Reconstruct 16-bit color from two bytes
                uint16_t wd = (buffer[2 * i] << 8) | buffer[2 * i + 1];

                if (wd != tc)
                { // If the pixel is not transparent
                    if (draw_start == -1)
                    {
                        draw_start = i; // Start new draw segment
                    }
                }
                else
                { // Transparent pixel
                    if (draw_start != -1)
                    {
                        // Render segment up to current pixel
                        tft.pushImage(pos_x + draw_start, pos_y + a, i - draw_start, 1, (uint16_t *)(&buffer[2 * draw_start]));
                        draw_start = -1; // Reset draw_start
                    }
                }
            }

            // Handle case where last segment reaches the end of the row
            if (draw_start != -1)
            {
                tft.pushImage(pos_x + draw_start, pos_y + a, size_x - draw_start, 1, (uint16_t *)(&buffer[2 * draw_start]));
            }
        }
    }
    file.close();
    fastMode(false);
}

void DRIVER::drawFromSd(uint32_t pos, int pos_x, int pos_y, int size_x, int size_y, bool transp, uint16_t tc, String file_path)
{
    drawFromSd(pos, pos_x, pos_y, size_x, size_y, file_path, transp, tc);
}

void DRIVER::drawFromSd(int x, int y, SDImage sprite, String file_path)
{
    drawFromSd(sprite.address, x, y, sprite.w, sprite.h, sprite.transp, sprite.tc, file_path);
}

void DRIVER::returnToSystem()
{
    ESP.restart();
}
