#include "driver.h"

#define DIB_MS 1000
#define RESOURCE_ADDRESS 0 
#define INPUT_LOCATION_Y 200
MCP23017 mcp = MCP23017(MCP23017_ADDR);
TFT_eSPI tft = TFT_eSPI();

void DRIVER::chfont(const GFXfont *f, bool is_screen_buffer, TFT_eSprite &sbuffer)
{
    if (is_screen_buffer)
    {
        sbuffer.setFreeFont(f);
    }
    else
    {
        tft.setFreeFont(f);
    }
}

void DRIVER::chfont(uint8_t f, bool is_screen_buffer, TFT_eSprite &sbuffer)
{
    if (is_screen_buffer)
    {
        sbuffer.setTextFont(f);
    }
    else
    {
        tft.setTextFont(f);
    }
}

void DRIVER::changeFont(int ch, bool is_screen_buffer, TFT_eSprite &sbuffer)
{
    this->currentFont = ch;
    switch (ch)
    {
    case 0:
        chfont(1, is_screen_buffer, sbuffer);
        break;
    case 1:
        chfont(&FreeSans9pt7b, is_screen_buffer, sbuffer);
        break;
    case 2:
        chfont(&FreeSansBold9pt7b, is_screen_buffer, sbuffer);
        break;
    case 3:
        chfont(&FreeMono9pt7b, is_screen_buffer, sbuffer);
        break;
    case 4:
        chfont(&FreeSans12pt7b, is_screen_buffer, sbuffer);
        break;
    }
}

void DRIVER::init()
{
    esp_ota_set_boot_partition(esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "app0"));

    pinMode(TFT_BL, OUTPUT);
    analogWrite(TFT_BL, 0);

    Wire.begin(21, 22);
    mcp.init();
    mcp.writeRegister(MCP23017Register::GPIO_A, 0x00); // Reset port A
    mcp.writeRegister(MCP23017Register::GPIO_B, 0x00); // Reset port B
    tft.init();
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

void DRIVER::drawCutoutFromSd(SDImage image,
                              int cutout_x, int cutout_y,
                              int cutout_width, int cutout_height,
                              int display_x, int display_y,
                              String file_path)
{
    fastMode(true);

    // Open the file from the SD card
    File file = SD.open(file_path);
    if (!file || !file.available())
    {
        sysError("FILE_NOT_AVAILABLE");
        return;
    }

    int image_width = image.w;

    // Calculate the starting offset for the cutout in the file
    uint32_t start_offset = image.address + (cutout_y * image_width + cutout_x) * 2;

    const int buffer_size = cutout_width * 2; // 2 bytes per pixel (16-bit color)
    uint8_t buffer[buffer_size];

    for (int row = 0; row < cutout_height; row++)
    {
        // Calculate the offset for the current row
        uint32_t row_offset = start_offset + (row * image_width * 2);
        file.seek(row_offset);

        // Read the row of pixel data into the buffer
        int bytesRead = file.read(buffer, buffer_size);
        if (bytesRead != buffer_size)
        {
            Serial.println("Error reading row from SD card.");
            return;
        }
        tft.pushImage(display_x, display_y + row, cutout_width, 1, (uint16_t *)buffer);
    }
    file.close();
    fastMode(false);
}

void DRIVER::drawFromSd(uint32_t pos, int pos_x, int pos_y, int size_x, int size_y, bool is_screen_buffer, TFT_eSprite &sbuffer, String file_path, bool transp, uint16_t tc)
{
    if (pos == 0)
        return;

    fastMode(true);
    if (file_path != resPath)
    {
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
    }
    else
    {
        pos -= RESOURCE_ADDRESS;
        // Serial.printf("POSITION %d\n", pos);

        if (resources)
        {

            uint16_t *imgData = (uint16_t *)(resources + (pos & ~1));
            if (!transp)
            {

                if (is_screen_buffer)
                    sbuffer.pushImage(pos_x, pos_y, size_x, size_y, imgData);
                else
                    tft.pushImage(pos_x, pos_y, size_x, size_y, imgData);
            }
            else
            {

                if (!sprite.createSprite(size_x, size_y))
                {
                    Serial.println("Sprite allocation failed!");
                    return;
                }

                sprite.pushImage(0, 0, size_x, size_y, imgData);

                if (is_screen_buffer)
                    sprite.pushToSprite(&sbuffer, pos_x, pos_y, tc);
                else
                    sprite.pushSprite(pos_x, pos_y, tc);

                sprite.deleteSprite();
            }
        }
    }

    fastMode(false);
}

void DRIVER::drawFromSd(uint32_t pos, int pos_x, int pos_y, int size_x, int size_y, String file_path, bool transp, uint16_t tc)
{
    drawFromSd(pos, pos_x, pos_y, size_x, size_y, false, screen_buffer, file_path, transp, tc);
}
void DRIVER::drawFromSd(uint32_t pos, int pos_x, int pos_y, int size_x, int size_y, bool transp, uint16_t tc)
{
    drawFromSd(pos, pos_x, pos_y, size_x, size_y, false, screen_buffer, resPath, transp, tc);
}
void DRIVER::drawFromSd(int x, int y, SDImage sprite, bool is_screen_buffer, TFT_eSprite &sbuffer)
{
    if (sprite.address != 0)
        drawFromSd(sprite.address, x, y, sprite.w, sprite.h, is_screen_buffer, sbuffer, resPath, sprite.transp, sprite.tc);
}
void DRIVER::drawFromSd(int x, int y, SDImage sprite)
{
    if (sprite.address != 0)
        drawFromSd(sprite.address, x, y, sprite.w, sprite.h, sprite.transp, sprite.tc);
}

char DRIVER::textInput(int input, bool onlynumbers, bool nonl)
{
    if (input == -1)
        return 0;
    char buttons[12][12] = {
        " \b0+@\n\r",
        "1,.?!()\r",
        "2ABCabc\r",
        "3DEFdef\r",
        "4GHIghi\r",
        "5JKLjkl\r",
        "6MNOmno\r",
        "7PQRSpqrs\r",
        "8TUVtuv\r",
        "9WXYZwxyz\r",
        "*\r",
        "#\r"};

    // nonl - disable new line
    if (nonl)
    {
        buttons[0][5] = '\r';
    }
    if (onlynumbers)
    {
        buttons[0][2] = '\r';
        for (int i = 1; i < 12; i++)
        {
            buttons[i][1] = '\r';
        }
    }
    bool first = true;
    char result = 0;
    int pos = 0;
    int currentIndex =
        input >= '0' && input <= '9'
            ? input - 48
        : input == '*' ? 10
        : input == '#' ? 11
                       : -1;
    if (currentIndex == -1)
    {
        Serial.println("UNKNOWN BUTTON:" + String(input));
        return 0;
    }

    for (int i = 0; i < 12; i++)
    {
        int b = 0;
        for (; b < 12; b++)
        {
            if (buttons[i][b] == '\r')
                break;
        }

        b = 0;
    }
    int mil = millis();
    pos = -1;
    int curx = tft.getCursorX();
    int cury = tft.getCursorY();
    while (millis() - mil < DIB_MS)
    {
        curx = tft.getCursorX();
        cury = tft.getCursorY();
        int c = buttonsHelding();
        if (c == input || first)
        {
            if (pos < (int)(strchr(buttons[currentIndex], '\r') - buttons[currentIndex]))
            {
                mil = millis();
                pos++;
                result = buttons[currentIndex][pos];
                showText(buttons[currentIndex], pos);
                tft.setCursor(curx, cury);
            }
            else
            {
                mil = millis();
                pos = 0;
                result = buttons[currentIndex][pos];
                showText(buttons[currentIndex], pos);
                tft.setCursor(curx, cury);
            }
        }
        first = false;
    }

    tft.fillRect(0, 300, 240, 30, 0);

    tft.setCursor(curx, cury);
    bool viewport = false;
    int h = tft.getViewportHeight();
    int w = tft.getViewportWidth();
    int vx = tft.getViewportX();
    int vy = tft.getViewportY();

    if (tft.getViewportHeight() < 320)
    {
        tft.resetViewport();
        viewport = true;
    }

    drawCutoutFromSd(SDImage(0x639365, 240, 269, 0, false), 0, INPUT_LOCATION_Y - 51, 120, 20, 0, INPUT_LOCATION_Y);
    if (viewport)
    {
        tft.setViewport(vx, vy, w, h);
    }
    if (result == '\r')
    {
        return 0;
    }
    return result;
}
void DRIVER::
    showText(const char *text, int pos)
{

    int h = tft.getViewportHeight();
    int w = tft.getViewportWidth();
    int vx = tft.getViewportX();
    int vy = tft.getViewportY();
    bool viewport = false;
    if (tft.getViewportHeight() < 320)
    {
        tft.resetViewport();
        viewport = true;
    }

    int pfont = currentFont;
    int textColor = tft.textcolor;
    int textSize = tft.textsize;
    changeFont(0);
    tft.setTextSize(2);

    tft.setCursor(0, INPUT_LOCATION_Y);

    for (int i = 0; i < (int)(strchr(text, '\r') - text); i++)
    {

        if (i != pos)
            tft.setTextColor(0xFFFF, 0, true);
        else
            tft.setTextColor(0xFFFF, 0x001F, true);
        if (text[i] == '\n')
            tft.print("NL");
        else if (text[i] == '\b')
            tft.print("<-");
        else
            tft.print(text[i]);
    }

    tft.textcolor = textColor;
    changeFont(pfont);
    tft.setTextSize(textSize);
    if (viewport)
        tft.setViewport(vx, vy, w, h);
}
void DRIVER::returnToSystem()
{
    ESP.restart();
}
