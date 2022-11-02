#include <M5Stack.h>
#include <WiFi.h>
#include <math.h>
#include "utility/M5Timer.h"
#include "time.h"
#include <Adafruit_NeoPixel.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>


// POWERSAVE
uint8_t BRIGHTNESS = 100;
bool POWERSAVE = false;

boolean is_state_changed = true;
typedef enum
{
  APP_STATE_INDEFINITE = 0,
  APP_STATE_DATE_TIME_CLOCK = 1,
  APP_STATE_ALARM_CLOCK = 2,
  APP_STATE_COUNT_DOWN_TIMER = 3,
  APP_STATE_THERMOMETER = 4,
} app_state_e;

// Wi-Fi
// User Info Needed!!!
const char *ssid = "XXXX"; // Enter Your WiFi SSID here
const char *password = "XXXX"; // Enter your WiFi password here

//MQTT
WiFiClient espClient;
PubSubClient client(espClient);
float temper = 0;
// User Info Needed!!!
const char *mqtt_server = "XX.XX.XX.XX"; // Enter Your MQTT Server IP here
int mqttPort = 1883; // Enter Your MQTT Server Port here
const char *mqttUser = "xxxxx"; // Enter Your MQTT Server username here
const char *mqttPassword = "xxxxx"; // Enter Your MQTT Server password here
const char *m5ikea_topic = "m5ikea/command";

// NTP
const char *ntp_server_1st = "0.europe.pool.ntp.org";
const char *ntp_server_2nd = "time.google.com";
// User Info Needed!!!
const long gmt_offset_sec = 3 * 3600; // Change here: GMT+3 for Istanbul
const int daylight_offset_sec = 0;    // Change here: No DST for Istanbul

// Acceleration sensor
M5Timer Task_Timer;

// LCD
#define LCD_LARGE_BAR_WIDTH (10)
#define LCD_LARGE_BAR_LENGTH (30)
#define LCD_LARGE_BAR_CORNER_RADIUS (6)
#define LCD_LARGE_BAR_GAP (LCD_LARGE_BAR_WIDTH >> 1)

#define LCD_SMALL_BAR_WIDTH (4)
#define LCD_SMALL_BAR_LENGTH (12)
#define LCD_SMALL_BAR_CORNER_RADIUS (3)
#define LCD_SMALL_BAR_GAP (LCD_SMALL_BAR_WIDTH >> 1)

#define LCD_DIGITS_CLEAR_ELM_NO (8)

// RGB Leds
#define PIN 15
#define NUMPIXELS 10
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB);
int delayval = 500;

const uint16_t digits_V_long[] =
    {
        0b0000001111111111, // 0
        0b0000001111000000, // 1
        0b0000011100111001, // 2
        0b0000011111100001, // 3
        0b0000011111000110, // 4
        0b0000010011100111, // 5
        0b0000010011111111, // 6
        0b0000001111000111, // 7
        0b0000011111111111, // 8
        0b0000011111100111, // 9
        0b0000000000000000, // off
};

// The screen resolution is 320 wide x 240 high, with the origin (0, 0) at the top left.
void drawNumberVLong(uint8_t x_start, uint8_t y_start, uint8_t number, uint8_t bar_width, uint8_t bar_length, uint8_t bar_gap, uint8_t corner_radius, uint16_t color_value)
{
  if (number > 10)
  {
    number = 10;
  }
  // top
  if (digits_V_long[number] & 0b0000000000000001)
    M5.Lcd.fillRoundRect(x_start, y_start, bar_length, bar_width, corner_radius, color_value);
  // upper-left
  if (digits_V_long[number] & 0b0000000000000010)
    M5.Lcd.fillRoundRect((x_start - bar_gap * 2), (y_start + bar_gap), bar_width, bar_length, corner_radius, color_value);
  if (digits_V_long[number] & 0b0000000000000100)
    M5.Lcd.fillRoundRect((x_start - bar_gap * 2), (y_start + bar_gap + bar_length * 1), bar_width, bar_length, corner_radius, color_value);
  // under-left
  if (digits_V_long[number] & 0b0000000000001000)
    M5.Lcd.fillRoundRect((x_start - bar_gap * 2), (y_start + bar_gap + bar_length * 2), bar_width, bar_length, corner_radius, color_value);
  if (digits_V_long[number] & 0b0000000000010000)
    M5.Lcd.fillRoundRect((x_start - bar_gap * 2), (y_start + bar_gap + bar_length * 3), bar_width, bar_length, corner_radius, color_value);
  // bottom
  if (digits_V_long[number] & 0b0000000000100000)
    M5.Lcd.fillRoundRect(x_start, (y_start + bar_length * 4), bar_length, bar_width, corner_radius, color_value);
  // under-right
  if (digits_V_long[number] & 0b0000000001000000)
    M5.Lcd.fillRoundRect((x_start + bar_length), (y_start + bar_gap + bar_length * 3), bar_width, bar_length, corner_radius, color_value);
  if (digits_V_long[number] & 0b0000000010000000)
    M5.Lcd.fillRoundRect((x_start + bar_length), (y_start + bar_gap + bar_length * 2), bar_width, bar_length, corner_radius, color_value);
  // upper-right
  if (digits_V_long[number] & 0b0000000100000000)
    M5.Lcd.fillRoundRect((x_start + bar_length), (y_start + bar_gap + bar_length * 1), bar_width, bar_length, corner_radius, color_value);
  if (digits_V_long[number] & 0b0000001000000000)
    M5.Lcd.fillRoundRect((x_start + bar_length), (y_start + bar_gap), bar_width, bar_length, corner_radius, color_value);
  // center
  if (digits_V_long[number] & 0b0000010000000000)
    M5.Lcd.fillRoundRect(x_start, (y_start + bar_length * 2), bar_length, bar_width, corner_radius, color_value);
}

const uint8_t digits_normal[] =
    {
        0b00111111, // 0
        0b00110000, // 1
        0b01101101, // 2
        0b01111001, // 3
        0b01110010, // 4
        0b01011011, // 5
        0b01011111, // 6
        0b00110011, // 7
        0b01111111, // 8
        0b01111011, // 9
        0b00000000, // off
};

void drawNumberNormal(uint8_t x_start, uint8_t y_start, uint8_t number, uint8_t bar_width, uint8_t bar_length, uint8_t bar_gap, uint8_t corner_radius, uint16_t color_value)
{
  if (number > 10)
  {
    number = 10;
  }
  // top
  if (digits_normal[number] & 0b0000000000000001)
    M5.Lcd.fillRoundRect(x_start, y_start, bar_length, bar_width, corner_radius, color_value);
  // upper-left
  if (digits_normal[number] & 0b0000000000000010)
    M5.Lcd.fillRoundRect((x_start - bar_gap * 2), (y_start + bar_gap), bar_width, bar_length, corner_radius, color_value);
  // under-left
  if (digits_normal[number] & 0b0000000000000100)
    M5.Lcd.fillRoundRect((x_start - bar_gap * 2), (y_start + bar_gap + bar_length * 1), bar_width, bar_length, corner_radius, color_value);
  // bottom
  if (digits_normal[number] & 0b0000000000001000)
    M5.Lcd.fillRoundRect(x_start, (y_start + bar_length * 2), bar_length, bar_width, corner_radius, color_value);
  // under-right
  if (digits_normal[number] & 0b0000000000010000)
    M5.Lcd.fillRoundRect((x_start + bar_length), (y_start + bar_gap + bar_length * 1), bar_width, bar_length, corner_radius, color_value);
  // upper-right
  if (digits_normal[number] & 0b0000000000100000)
    M5.Lcd.fillRoundRect((x_start + bar_length), (y_start + bar_gap), bar_width, bar_length, corner_radius, color_value);
  // center
  if (digits_normal[number] & 0b0000000001000000)
    M5.Lcd.fillRoundRect(x_start, (y_start + bar_length * 1), bar_length, bar_width, corner_radius, color_value);
}

// Blink for screen icons, RGB Leds and Speaker Alarm
#define LCD_DISP_BLINK_LOOP_CNT (5)
class BlinkCount
{
private:
  uint32_t count;

public:
  void incrementCount(void);
  void resetCount(void);
  boolean isHideDisplay(void);
};

void BlinkCount::incrementCount(void)
{
  ++count;
}

void BlinkCount::resetCount(void)
{
  count = 0;
}

boolean BlinkCount::isHideDisplay(void)
{
  if ((count / LCD_DISP_BLINK_LOOP_CNT % 2) == 0)
  {
    return false;
  }
  else
  {
    return true;
  }
}

BlinkCount cl_blink_count;

// Powersave Dim Screen Timeout
#define POWERSAVE_LOOP_CNT (150)
class PowerCount
{
private:
  uint32_t count;

public:
  void incrementCount(void);
  void resetCount(void);
  boolean isDimDisplay(void);
  int counter(void);
};

void PowerCount::incrementCount(void)
{
  ++count;
}

void PowerCount::resetCount(void)
{
  count = 0;
}

int PowerCount::counter(void)
{
  return count;
}

boolean PowerCount::isDimDisplay(void)
{
  if (count > POWERSAVE_LOOP_CNT)
  {
    return false;
  }
  else
  {
    return true;
  }
}

PowerCount pw_save_count;
bool isModulePluggedIn = false;
uint8_t Bat_Level = 0;

// System Clock
class SystemClock
{
private:
  struct tm time_info;
  time_t timer;

public:
  uint32_t year = 0;
  uint32_t month = 0;
  uint32_t day = 0;
  uint32_t hour = 0;
  uint32_t minute = 0;
  uint32_t week_day = 0;
  uint32_t second = 0;
  uint32_t prev_year = 0;
  uint32_t prev_month = 0;
  uint32_t prev_day = 0;
  uint32_t prev_hour = 0;
  uint32_t prev_minute = 0;
  uint32_t prev_week_day = 0;
  uint32_t prev_second = 0;
  void backupCurrentTime(void);
  void updateByNtp(void);
  void updateBySoftTimer(uint32_t elapsed_second);
};

void SystemClock::backupCurrentTime(void)
{
  prev_year = year;
  prev_month = month;
  prev_day = day;
  prev_hour = hour;
  prev_minute = minute;
  prev_week_day = week_day;
  prev_second = second;
}

void SystemClock::updateBySoftTimer(uint32_t elapsed_second)
{
  struct tm *local_time;
  time_t timer_add = timer + elapsed_second;
  local_time = localtime(&timer_add);
  year = local_time->tm_year + 1900;
  month = local_time->tm_mon + 1;
  day = local_time->tm_mday;
  hour = local_time->tm_hour;
  minute = local_time->tm_min;
  week_day = local_time->tm_wday;
  second = local_time->tm_sec;
}

void SystemClock::updateByNtp(void)
{
  Serial.println("---NTP ACCESS---");
  if (!getLocalTime(&time_info))
  {
    year = 0;
    month = 0;
    day = 0;
    hour = 0;
    minute = 0;
    week_day = 0;
    second = 0;
    timer = 0;
  }
  else
  {
    year = time_info.tm_year + 1900;
    month = time_info.tm_mon + 1;
    day = time_info.tm_mday;
    hour = time_info.tm_hour;
    minute = time_info.tm_min;
    week_day = time_info.tm_wday;
    second = time_info.tm_sec;
    timer = mktime(&time_info);
  }
}

SystemClock cl_system_clock;
#define NTP_ACCESS_MS_INTERVAL (300000)

#define LCD_CLOCK_YMD_DISP_Y_POS (10)
#define LCD_CLOCK_MD_STR_DISP_Y_POS (45)
#define LCD_CLOCK_HM_DISP_Y_POS (100)
#define LCD_CLOCK_PM_STR_DISP_Y_POS (175)
#define LCD_CLOCK_ICON_DISP_Y_POS (200)
#define LCD_CLOCK_WEEK_STR_DISP_Y_POS (220)

#define LCD_CLOCK_BATTERY_X_POS (270)
#define LCD_CLOCK_BATTERY_Y_POS (50)

void displayDateTimeScreen()
{
  static boolean ntp_access_flag = true;
  static uint32_t base_milli_time;
  uint32_t elapsed_second = 0;
  uint32_t diff_milli_time = 0;

  uint16_t bkground_color = M5.Lcd.color565(200, 0, 0);
  if (is_state_changed == true)
  {
    M5.Lcd.fillScreen(bkground_color);
    ntp_access_flag = true;
    pw_save_count.resetCount();
    is_state_changed = false;
  }

  if (ntp_access_flag == true)
  {
    base_milli_time = millis();
    Serial.print("base_milli_time:");
    Serial.println(base_milli_time);
    cl_system_clock.updateByNtp();
    ntp_access_flag = false;
  }
  else
  {
    diff_milli_time = millis() - base_milli_time;
    if (diff_milli_time > NTP_ACCESS_MS_INTERVAL)
    {
      ntp_access_flag = true;
    }
    elapsed_second = diff_milli_time / 1000;
    cl_system_clock.updateBySoftTimer(elapsed_second);
  }

  M5.Lcd.setTextColor(TFT_BLACK);
  M5.Lcd.setTextSize(2);

  // Month
  if (cl_system_clock.month != cl_system_clock.prev_month)
  {
    drawNumberNormal(80, LCD_CLOCK_YMD_DISP_Y_POS, LCD_DIGITS_CLEAR_ELM_NO, LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, bkground_color);
    drawNumberNormal(105, LCD_CLOCK_YMD_DISP_Y_POS, LCD_DIGITS_CLEAR_ELM_NO, LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, bkground_color);
  }
  drawNumberNormal(80, LCD_CLOCK_YMD_DISP_Y_POS, (cl_system_clock.month / 10), LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, TFT_BLACK);
  drawNumberNormal(105, LCD_CLOCK_YMD_DISP_Y_POS, (cl_system_clock.month % 10), LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, TFT_BLACK);

  //M5.Lcd.drawLine(60, LCD_SMALL_BAR_LENGTH * 2 + 10, 70, 10, TFT_BLACK);
  M5.Lcd.drawLine(60, 22, 70, 22, TFT_BLACK);
  // Day
  if (cl_system_clock.day != cl_system_clock.prev_day)
  {
    drawNumberNormal(10, LCD_CLOCK_YMD_DISP_Y_POS, LCD_DIGITS_CLEAR_ELM_NO, LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, bkground_color);
    drawNumberNormal(35, LCD_CLOCK_YMD_DISP_Y_POS, LCD_DIGITS_CLEAR_ELM_NO, LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, bkground_color);
  }
  drawNumberNormal(10, LCD_CLOCK_YMD_DISP_Y_POS, (cl_system_clock.day / 10), LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, TFT_BLACK);
  drawNumberNormal(35, LCD_CLOCK_YMD_DISP_Y_POS, (cl_system_clock.day % 10), LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, TFT_BLACK);

  // Year
  if (cl_system_clock.year != cl_system_clock.prev_year)
  {
    drawNumberNormal(180, LCD_CLOCK_YMD_DISP_Y_POS, LCD_DIGITS_CLEAR_ELM_NO, LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, bkground_color);
    drawNumberNormal(205, LCD_CLOCK_YMD_DISP_Y_POS, LCD_DIGITS_CLEAR_ELM_NO, LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, bkground_color);
    drawNumberNormal(230, LCD_CLOCK_YMD_DISP_Y_POS, LCD_DIGITS_CLEAR_ELM_NO, LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, bkground_color);
    drawNumberNormal(255, LCD_CLOCK_YMD_DISP_Y_POS, LCD_DIGITS_CLEAR_ELM_NO, LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, bkground_color);
  }
  drawNumberNormal(180, LCD_CLOCK_YMD_DISP_Y_POS, (cl_system_clock.year / 1000), LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, TFT_BLACK);
  drawNumberNormal(205, LCD_CLOCK_YMD_DISP_Y_POS, ((cl_system_clock.year % 1000) / 100), LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, TFT_BLACK);
  drawNumberNormal(230, LCD_CLOCK_YMD_DISP_Y_POS, (((cl_system_clock.year % 1000) % 100) / 10), LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, TFT_BLACK);
  drawNumberNormal(255, LCD_CLOCK_YMD_DISP_Y_POS, (((cl_system_clock.year % 1000) % 100) % 10), LCD_SMALL_BAR_WIDTH, LCD_SMALL_BAR_LENGTH, LCD_SMALL_BAR_GAP, LCD_SMALL_BAR_CORNER_RADIUS, TFT_BLACK);

  // hour
  if (cl_system_clock.hour != cl_system_clock.prev_hour)
  {
    drawNumberNormal(30, LCD_CLOCK_HM_DISP_Y_POS, LCD_DIGITS_CLEAR_ELM_NO, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
    drawNumberNormal(95, LCD_CLOCK_HM_DISP_Y_POS, LCD_DIGITS_CLEAR_ELM_NO, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
    M5.Lcd.setTextColor(bkground_color);
    M5.Lcd.drawString("PM", 20, LCD_CLOCK_PM_STR_DISP_Y_POS);
    M5.Lcd.setTextColor(TFT_BLACK);
  }
  drawNumberNormal(30, LCD_CLOCK_HM_DISP_Y_POS, (cl_system_clock.hour / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
  drawNumberNormal(95, LCD_CLOCK_HM_DISP_Y_POS, (cl_system_clock.hour % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);

  if (cl_blink_count.isHideDisplay() == false)
  {
    M5.Lcd.fillEllipse(150, 120, 4, 4, TFT_BLACK);
    M5.Lcd.fillEllipse(150, 150, 4, 4, TFT_BLACK);
  }
  else
  {
    M5.Lcd.fillEllipse(150, 120, 4, 4, bkground_color);
    M5.Lcd.fillEllipse(150, 150, 4, 4, bkground_color);
  }

  // Battery Icon
  if ((isModulePluggedIn) && (M5.Power.isChargeFull() != true)) {
    if (cl_blink_count.isHideDisplay() == false) {
      drawBatteryIcon(LCD_CLOCK_BATTERY_X_POS, LCD_CLOCK_BATTERY_Y_POS, TFT_BLACK);
    }
    else {
      drawBatteryIcon(LCD_CLOCK_BATTERY_X_POS, LCD_CLOCK_BATTERY_Y_POS, bkground_color);
    }        
  } else {
    drawBatteryIcon(LCD_CLOCK_BATTERY_X_POS, LCD_CLOCK_BATTERY_Y_POS, TFT_BLACK);
  }

  // minute
  if (cl_system_clock.minute != cl_system_clock.prev_minute)
  {
    drawNumberNormal(185, LCD_CLOCK_HM_DISP_Y_POS, LCD_DIGITS_CLEAR_ELM_NO, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
    drawNumberNormal(250, LCD_CLOCK_HM_DISP_Y_POS, LCD_DIGITS_CLEAR_ELM_NO, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
  }
  drawNumberNormal(185, LCD_CLOCK_HM_DISP_Y_POS, (cl_system_clock.minute / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
  drawNumberNormal(250, LCD_CLOCK_HM_DISP_Y_POS, (cl_system_clock.minute % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);

  // MONTH DATE
  const String s_month = "MONTH";
  const String s_date = "DATE";
  M5.Lcd.drawString(s_month, 10, LCD_CLOCK_MD_STR_DISP_Y_POS);
  M5.Lcd.drawString(s_date, 80, LCD_CLOCK_MD_STR_DISP_Y_POS);

  uint8_t week_count = 0;
  const char aweek[7][4] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  uint8_t day_week_now = 0;
  day_week_now = cl_system_clock.week_day;
  for (week_count = 0; week_count < 7; ++week_count)
  {
    if ((week_count == day_week_now) && (cl_blink_count.isHideDisplay() == true))
    {
      M5.Lcd.setTextColor(bkground_color);
      M5.Lcd.drawString(aweek[week_count], week_count * 45 + 10, LCD_CLOCK_WEEK_STR_DISP_Y_POS);
      M5.Lcd.setTextColor(TFT_BLACK);
    }
    else
    {
      M5.Lcd.drawString(aweek[week_count], week_count * 45 + 10, LCD_CLOCK_WEEK_STR_DISP_Y_POS);
    }
  }

  // clock icon
  drawClockIcon(300, LCD_CLOCK_ICON_DISP_Y_POS);


  // t_date_prev = t_date_now;
  cl_system_clock.backupCurrentTime();
}

// Thermometer Screen
#define LCD_THERMOMETER_ICON_DISP_Y_POS (80)
#define LCD_THERMOMETER_TEMPERATURE_DISP_Y_POS (80)

void displayThermometerScreen()
{
  static uint8_t temperature_prev = 0;
  bool minusDegree = false;
  uint16_t bkground_color = M5.Lcd.color565(100, 110, 255);//Blue

  if (temper < 0) minusDegree = true;

  uint8_t temperature = abs(temper); 
  uint8_t tens_place, ones_place;

  // Background color according to temperature:
  if (temperature < 10)
  {
    tens_place = 0;
    ones_place = temperature;
    bkground_color = M5.Lcd.color565(100, 110, 255);// Dark Blue
  }
  else if (temperature <= 99)
  {
    tens_place = temperature / 10;
    ones_place = temperature % 10;
    if (temperature <= 18) {
      bkground_color = M5.Lcd.color565(128, 128, 255);//Light Blue
    } 
    else if (temperature <= 24) {
      bkground_color = M5.Lcd.color565(255, 255, 0);// Yellow
    }
    else if (temperature <= 32) {
      bkground_color = M5.Lcd.color565(255, 102, 102);//Light red
    } else {
      bkground_color = M5.Lcd.color565(255, 0, 0);//Dark red
    }
  }
  else
  {
    tens_place = 9;
    ones_place = 9;
  }

  if (minusDegree) bkground_color = M5.Lcd.color565(100, 110, 255); // Blue for all minus degrees

  if (is_state_changed == true)
  {
    M5.Lcd.fillScreen(bkground_color);
    pw_save_count.resetCount();
    is_state_changed = false;
  }

  if (temperature != temperature_prev)
  {
    M5.Lcd.fillScreen(bkground_color);
  }
  // draw thermometer icon
  drawThermometerIcon(40, LCD_THERMOMETER_ICON_DISP_Y_POS, bkground_color);

  if (minusDegree) M5.Lcd.fillRoundRect(20, 145, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);

  if (temperature != temperature_prev)
  {
    drawNumberVLong(75, LCD_THERMOMETER_TEMPERATURE_DISP_Y_POS, LCD_DIGITS_CLEAR_ELM_NO, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
    drawNumberVLong(145, LCD_THERMOMETER_TEMPERATURE_DISP_Y_POS, LCD_DIGITS_CLEAR_ELM_NO, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
  }
  drawNumberVLong(75, LCD_THERMOMETER_TEMPERATURE_DISP_Y_POS, tens_place, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
  drawNumberVLong(145, LCD_THERMOMETER_TEMPERATURE_DISP_Y_POS, ones_place, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);

  M5.Lcd.drawEllipse(145 + LCD_LARGE_BAR_WIDTH + LCD_LARGE_BAR_LENGTH + LCD_LARGE_BAR_GAP, LCD_THERMOMETER_TEMPERATURE_DISP_Y_POS, 3, 3, TFT_BLACK);
  M5.Lcd.drawChar(145 + LCD_LARGE_BAR_WIDTH + LCD_LARGE_BAR_LENGTH + LCD_LARGE_BAR_GAP + 10, LCD_THERMOMETER_TEMPERATURE_DISP_Y_POS, 'C', TFT_BLACK, bkground_color, 4);

  temperature_prev = temperature;
}

// Timer Screen
#define LCD_SANDGLASS_ICON_DISP_Y_POS (30)
#define LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS (70)
#define LCD_CNT_DOWN_TIMER_STR_DISP_Y_POS (150)
#define LCD_CNT_DOWN_TIMER_ICON_DISP_Y_POS (200)

typedef enum
{
  CNT_DOWN_TIMER_NOT_SET = 0,
  CNT_DOWN_TIMER_RUNNING = 1,
  CNT_DOWN_TIMER_SETTING_MINUTE = 2,
  CNT_DOWN_TIMER_SETTING_SECOND = 3,
  CNT_DOWN_TIMER_RINGING = 4,
} count_down_timer_status_e;

void displayCountdownTimerScreen()
{
  static count_down_timer_status_e e_cnt_timer_status = CNT_DOWN_TIMER_NOT_SET;
  static uint8_t timer_minute = 0;
  static uint8_t timer_second = 0;
  static uint32_t base_milli_time = 0;
  static uint32_t set_milli_time = 0;
  static boolean is_count_down = false;

  uint16_t bkground_color = M5.Lcd.color565(0, 180, 0);
  if (is_state_changed == true)
  {
    M5.Lcd.fillScreen(bkground_color);
    pw_save_count.resetCount();
    is_state_changed = false;
    is_count_down = false;
    e_cnt_timer_status = CNT_DOWN_TIMER_RUNNING;
  }

  // draw sandglass icon
  if (cl_blink_count.isHideDisplay() == false)
  {
    drawSandglassIcon(240, LCD_SANDGLASS_ICON_DISP_Y_POS, TFT_BLACK);
  }
  else
  {
    drawSandglassIcon(240, LCD_SANDGLASS_ICON_DISP_Y_POS, bkground_color);
  }

  M5.Lcd.setTextColor(TFT_BLACK);
  M5.Lcd.setTextSize(2);

  M5.Lcd.drawString("MIN", 20, LCD_CNT_DOWN_TIMER_STR_DISP_Y_POS);
  M5.Lcd.drawString("SEC", 250, LCD_CNT_DOWN_TIMER_STR_DISP_Y_POS);

  switch (e_cnt_timer_status)
  {
  case CNT_DOWN_TIMER_NOT_SET:
  default:
    drawNumberNormal(30, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, 0, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    drawNumberNormal(95, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, 0, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    M5.Lcd.fillEllipse(150, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS + 20, 4, 4, TFT_BLACK);
    M5.Lcd.fillEllipse(150, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS + 50, 4, 4, TFT_BLACK);
    drawNumberNormal(185, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, 0, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    drawNumberNormal(250, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, 0, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);

    if (M5.BtnA.wasReleased())
    {
      e_cnt_timer_status = CNT_DOWN_TIMER_SETTING_MINUTE;
      M5.Lcd.fillScreen(bkground_color);
    }
    break;

  case CNT_DOWN_TIMER_SETTING_MINUTE:
    if (cl_blink_count.isHideDisplay() == false)
    {
      drawNumberNormal(30, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_minute / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
      drawNumberNormal(95, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_minute % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    }
    else
    {
      drawNumberNormal(30, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_minute / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
      drawNumberNormal(95, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_minute % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
    }
    M5.Lcd.fillEllipse(150, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS + 20, 4, 4, TFT_BLACK);
    M5.Lcd.fillEllipse(150, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS + 50, 4, 4, TFT_BLACK);
    drawNumberNormal(185, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_second / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    drawNumberNormal(250, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_second % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);

    if (M5.BtnC.wasReleased())
    {
      ++timer_minute;
      if (timer_minute > 99)
      {
        timer_minute = 0;
      }
      M5.Lcd.fillScreen(bkground_color);
    }
    else if (M5.BtnA.wasReleased())
    {
      e_cnt_timer_status = CNT_DOWN_TIMER_SETTING_SECOND;
      M5.Lcd.fillScreen(bkground_color);
    }
    break;

  case CNT_DOWN_TIMER_SETTING_SECOND:
    drawNumberNormal(30, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_minute / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    drawNumberNormal(95, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_minute % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    M5.Lcd.fillEllipse(150, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS + 20, 4, 4, TFT_BLACK);
    M5.Lcd.fillEllipse(150, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS + 50, 4, 4, TFT_BLACK);
    if (cl_blink_count.isHideDisplay() == false)
    {
      drawNumberNormal(185, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_second / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
      drawNumberNormal(250, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_second % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    }
    else
    {
      drawNumberNormal(185, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_second / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
      drawNumberNormal(250, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_second % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
    }

    if (M5.BtnC.wasReleased())
    {
      ++timer_second;
      if (timer_second > 59)
      {
        timer_second = 0;
      }
      M5.Lcd.fillScreen(bkground_color);
    }
    else if (M5.BtnA.wasReleased())
    {
      e_cnt_timer_status = CNT_DOWN_TIMER_RUNNING;
      M5.Lcd.fillScreen(bkground_color);
      cl_blink_count.resetCount();
    }
    break;

  case CNT_DOWN_TIMER_RUNNING:
    if ((timer_minute == 0) && (timer_second == 0))
    {
      e_cnt_timer_status = CNT_DOWN_TIMER_NOT_SET;
    }
    else
    {
      uint8_t disp_minute = 0;
      uint8_t disp_second = 0;
      static uint8_t pre_disp_minute = 0;
      static uint8_t pre_disp_second = 0;
      if (is_count_down == false)
      {
        is_count_down = true;
        base_milli_time = millis();
        set_milli_time = base_milli_time + timer_second * 1000 + timer_minute * 60 * 1000;

        drawNumberNormal(30, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_minute / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
        drawNumberNormal(95, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_minute % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
        M5.Lcd.fillEllipse(150, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS + 20, 4, 4, TFT_BLACK);
        M5.Lcd.fillEllipse(150, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS + 50, 4, 4, TFT_BLACK);
        drawNumberNormal(185, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_second / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
        drawNumberNormal(250, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (timer_second % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
        pre_disp_minute = timer_minute;
        pre_disp_second = timer_second;
      }
      else
      {
        int32_t diff_milli_time = set_milli_time - millis();
        if (diff_milli_time < 0)
        {
          e_cnt_timer_status = CNT_DOWN_TIMER_RINGING;
          is_count_down = false;
        }
        else
        {
          disp_minute = diff_milli_time / 1000 / 60;
          disp_second = diff_milli_time / 1000 % 60;

          if (disp_minute != pre_disp_minute)
          {
            drawNumberNormal(30, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (pre_disp_minute / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
            drawNumberNormal(95, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (pre_disp_minute % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
          }
          if (disp_second != pre_disp_second)
          {
            drawNumberNormal(185, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (pre_disp_second / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
            drawNumberNormal(250, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (pre_disp_second % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
          }

          drawNumberNormal(30, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (disp_minute / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
          drawNumberNormal(95, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (disp_minute % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
          M5.Lcd.fillEllipse(150, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS + 20, 4, 4, TFT_BLACK);
          M5.Lcd.fillEllipse(150, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS + 50, 4, 4, TFT_BLACK);
          drawNumberNormal(185, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (disp_second / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
          drawNumberNormal(250, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, (disp_second % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);

          pre_disp_minute = disp_minute;
          pre_disp_second = disp_second;
        }
      }

      if (M5.BtnA.wasReleased())
      {
        e_cnt_timer_status = CNT_DOWN_TIMER_SETTING_MINUTE;
        timer_minute = 0;
        timer_second = 0;
        is_count_down = false;
        M5.Lcd.fillScreen(bkground_color);
        cl_blink_count.resetCount();
      }
    }
    break;

  case CNT_DOWN_TIMER_RINGING:
    drawNumberNormal(30, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, 0, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    drawNumberNormal(95, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, 0, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    M5.Lcd.fillEllipse(150, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS + 20, 4, 4, TFT_BLACK);
    M5.Lcd.fillEllipse(150, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS + 50, 4, 4, TFT_BLACK);
    drawNumberNormal(185, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, 0, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    drawNumberNormal(250, LCD_CNT_DOWN_TIMER_MS_DISP_Y_POS, 0, LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);

    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(4);
    M5.Lcd.drawString("RINGING", 20, 180);

    if (cl_blink_count.isHideDisplay() == false)
      {
        for(int i=0;i<NUMPIXELS;i++){
        // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
        pixels.setPixelColor(i, pixels.Color(0,0,150)); // Moderately bright BLUE color.
        pixels.show(); // This sends the updated pixel color to the hardware.
        // delay(delayval); // Delay for a period of time (in milliseconds).
        M5.Speaker.tone(294);
        }
      }
      else
      {
        pixels.clear();
        pixels.show();
        M5.Speaker.mute();
      }

    if (M5.BtnA.wasReleased())
    {
      e_cnt_timer_status = CNT_DOWN_TIMER_SETTING_MINUTE;
      timer_minute = 0;
      timer_second = 0;
      M5.Lcd.fillScreen(bkground_color);
      pixels.clear();
      pixels.show();  
      M5.Speaker.end();    
      cl_blink_count.resetCount();
    }
    else if (M5.BtnC.wasReleased())
    {
      e_cnt_timer_status = CNT_DOWN_TIMER_NOT_SET;
      timer_minute = 0;
      timer_second = 0;
      M5.Lcd.fillScreen(bkground_color);
      pixels.clear();
      pixels.show();
      M5.Speaker.end();
      cl_blink_count.resetCount();
    }
    break;
  }
}

// Alarm Screen
#define LCD_ALARM_HM_DISP_Y_POS (150)
#define LCD_ALARM_STR_DISP_Y_POS (100)
#define LCD_ALARM_CLOCK_ICON_DISP_Y_POS (105)
#define LCD_ALARM_BELL_ICON_DISP_Y_POS (110)

#define LCD_ALARM_H_DISP_TENS_PLACE_X_POS (15)
#define LCD_ALARM_H_DISP_ONES_PLACE_X_POS (70)
#define LCD_ALARM_M_DISP_TENS_PLACE_X_POS (140)
#define LCD_ALARM_M_DISP_ONES_PLACE_X_POS (195)
#define LCD_ALARM_HM_DISP_COLON_X_POS (120)

typedef enum
{
  ALARM_NOT_SET = 0,
  ALARM_RUNNING = 1,
  ALARM_SETTING_HOUR = 2,
  ALARM_SETTING_MINUTE = 3,
} alarm_status_e;
alarm_status_e e_alarm_status = ALARM_NOT_SET;

void displayAlarmScreen()
{
  static uint8_t alarm_hour = 0;
  static uint8_t alarm_minute = 0;
  static int16_t callback_alarm_slot_id = -1;

  uint16_t bkground_color = M5.Lcd.color565(0, 180, 0);
  if (is_state_changed == true)
  {
    M5.Lcd.fillScreen(bkground_color);
    pw_save_count.resetCount();
    is_state_changed = false;
  }

  M5.Lcd.setTextColor(TFT_BLACK);
  M5.Lcd.setTextSize(2);

  M5.Lcd.drawString("ALARM", 80, LCD_ALARM_STR_DISP_Y_POS);

  // alarm icon
  drawAlarmIcon(55, LCD_ALARM_CLOCK_ICON_DISP_Y_POS);

  switch (e_alarm_status)
  {
  case ALARM_NOT_SET:
  default:
    drawNumberNormal(LCD_ALARM_H_DISP_TENS_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_hour / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    drawNumberNormal(LCD_ALARM_H_DISP_ONES_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_hour % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    M5.Lcd.fillEllipse(LCD_ALARM_HM_DISP_COLON_X_POS, LCD_ALARM_HM_DISP_Y_POS + 20, 4, 4, TFT_BLACK);
    M5.Lcd.fillEllipse(LCD_ALARM_HM_DISP_COLON_X_POS, LCD_ALARM_HM_DISP_Y_POS + 50, 4, 4, TFT_BLACK);
    drawNumberNormal(LCD_ALARM_M_DISP_TENS_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_minute / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    drawNumberNormal(LCD_ALARM_M_DISP_ONES_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_minute % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);

    if (M5.BtnA.wasReleased())
    {
      e_alarm_status = ALARM_SETTING_HOUR;
      M5.Lcd.fillScreen(bkground_color);
    }
    break;

  case ALARM_SETTING_HOUR:
    if (cl_blink_count.isHideDisplay() == false)
    {
      drawNumberNormal(LCD_ALARM_H_DISP_TENS_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_hour / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
      drawNumberNormal(LCD_ALARM_H_DISP_ONES_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_hour % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    }
    else
    {
      drawNumberNormal(LCD_ALARM_H_DISP_TENS_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_hour / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
      drawNumberNormal(LCD_ALARM_H_DISP_ONES_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_hour % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
    }
    M5.Lcd.fillEllipse(LCD_ALARM_HM_DISP_COLON_X_POS, LCD_ALARM_HM_DISP_Y_POS + 20, 4, 4, TFT_BLACK);
    M5.Lcd.fillEllipse(LCD_ALARM_HM_DISP_COLON_X_POS, LCD_ALARM_HM_DISP_Y_POS + 50, 4, 4, TFT_BLACK);
    drawNumberNormal(LCD_ALARM_M_DISP_TENS_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_minute / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    drawNumberNormal(LCD_ALARM_M_DISP_ONES_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_minute % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);

    if (M5.BtnC.wasReleased())
    {
      ++alarm_hour;
      if (alarm_hour > 23)
      {
        alarm_hour = 0;
      }
      M5.Lcd.fillScreen(bkground_color);
    }
    else if (M5.BtnA.wasReleased())
    {
      e_alarm_status = ALARM_SETTING_MINUTE;
      M5.Lcd.fillScreen(bkground_color);
    }
    break;

  case ALARM_SETTING_MINUTE:
    drawNumberNormal(LCD_ALARM_H_DISP_TENS_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_hour / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    drawNumberNormal(LCD_ALARM_H_DISP_ONES_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_hour % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    M5.Lcd.fillEllipse(LCD_ALARM_HM_DISP_COLON_X_POS, LCD_ALARM_HM_DISP_Y_POS + 20, 4, 4, TFT_BLACK);
    M5.Lcd.fillEllipse(LCD_ALARM_HM_DISP_COLON_X_POS, LCD_ALARM_HM_DISP_Y_POS + 50, 4, 4, TFT_BLACK);
    if (cl_blink_count.isHideDisplay() == false)
    {
      drawNumberNormal(LCD_ALARM_M_DISP_TENS_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_minute / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
      drawNumberNormal(LCD_ALARM_M_DISP_ONES_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_minute % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    }
    else
    {
      drawNumberNormal(LCD_ALARM_M_DISP_TENS_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_minute / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
      drawNumberNormal(LCD_ALARM_M_DISP_ONES_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_minute % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, bkground_color);
    }

    if (M5.BtnC.wasReleased())
    {
      ++alarm_minute;
      if (alarm_minute > 59)
      {
        alarm_minute = 0;
      }
      M5.Lcd.fillScreen(bkground_color);
    }
    else if (M5.BtnA.wasReleased())
    {
      callback_alarm_slot_id = enableAlarm(alarm_hour, alarm_minute);
      e_alarm_status = ALARM_RUNNING;
      M5.Lcd.fillScreen(bkground_color);
    }
    break;

  case ALARM_RUNNING:
    drawNumberNormal(LCD_ALARM_H_DISP_TENS_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_hour / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    drawNumberNormal(LCD_ALARM_H_DISP_ONES_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_hour % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    M5.Lcd.fillEllipse(LCD_ALARM_HM_DISP_COLON_X_POS, LCD_ALARM_HM_DISP_Y_POS + 20, 4, 4, TFT_BLACK);
    M5.Lcd.fillEllipse(LCD_ALARM_HM_DISP_COLON_X_POS, LCD_ALARM_HM_DISP_Y_POS + 50, 4, 4, TFT_BLACK);
    drawNumberNormal(LCD_ALARM_M_DISP_TENS_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_minute / 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);
    drawNumberNormal(LCD_ALARM_M_DISP_ONES_PLACE_X_POS, LCD_ALARM_HM_DISP_Y_POS, (alarm_minute % 10), LCD_LARGE_BAR_WIDTH, LCD_LARGE_BAR_LENGTH, LCD_LARGE_BAR_GAP, LCD_LARGE_BAR_CORNER_RADIUS, TFT_BLACK);

    // bell icon
    drawBellIcon(20, LCD_ALARM_BELL_ICON_DISP_Y_POS);

    if (M5.BtnA.wasReleased())
    {
      disableAlarm(callback_alarm_slot_id);
      callback_alarm_slot_id = -1;
      e_alarm_status = ALARM_NOT_SET;
      M5.Lcd.fillScreen(bkground_color);
    }
    break;
  }
}

// Draw Battery Status Icon
void drawBatteryIcon(uint32_t base_x_pos, uint32_t base_y_pos, uint16_t color)
{
  // Battery Body
  M5.Lcd.drawRect(base_x_pos, base_y_pos, 36, 18, TFT_BLACK);
  // Battery Positive
  M5.Lcd.fillRect(base_x_pos-4, base_y_pos+5, 4, 8, TFT_BLACK);
  // Battery Bars
  if (Bat_Level > 0) M5.Lcd.fillRect(base_x_pos+27, base_y_pos+3, 6, 12, color);
  if (Bat_Level > 25) M5.Lcd.fillRect(base_x_pos+19, base_y_pos+3, 6, 12, color);
  if (Bat_Level > 50) M5.Lcd.fillRect(base_x_pos+11, base_y_pos+3, 6, 12, color);
  if (Bat_Level > 75) M5.Lcd.fillRect(base_x_pos+3, base_y_pos+3, 6, 12, color);
}

// Draw Icon API
void drawAlarmIcon(uint32_t base_x_pos, uint32_t base_y_pos)
{
  const int32_t offset = 14;

  M5.Lcd.drawEllipse(base_x_pos, base_y_pos, 7, 7, TFT_BLACK);

  M5.Lcd.drawLine(base_x_pos, base_y_pos, base_x_pos - 2, base_y_pos - 2, TFT_BLACK);
  M5.Lcd.drawLine(base_x_pos, base_y_pos + 1, base_x_pos - 2, base_y_pos - 1, TFT_BLACK);
  M5.Lcd.drawLine(base_x_pos, base_y_pos, base_x_pos + 4, base_y_pos - 3, TFT_BLACK);
  M5.Lcd.drawLine(base_x_pos, base_y_pos + 1, base_x_pos + 4, base_y_pos - 2, TFT_BLACK);

  M5.Lcd.drawLine(base_x_pos - 5, base_y_pos + 5, base_x_pos - 7, base_y_pos + 7, TFT_BLACK);
  M5.Lcd.drawLine(base_x_pos - 5, base_y_pos + 6, base_x_pos - 7, base_y_pos + 8, TFT_BLACK);
  M5.Lcd.drawLine(base_x_pos + 5, base_y_pos + 5, base_x_pos + 7, base_y_pos + 7, TFT_BLACK);
  M5.Lcd.drawLine(base_x_pos + 5, base_y_pos + 6, base_x_pos + 7, base_y_pos + 8, TFT_BLACK);

  M5.Lcd.fillRect(base_x_pos - 3, base_y_pos - 11, 6, 2, TFT_BLACK);
  M5.Lcd.fillRect(base_x_pos - 2, base_y_pos - 9, 4, 1, TFT_BLACK);

  M5.Lcd.fillTriangle(base_x_pos - 8, base_y_pos - 5, base_x_pos - 5, base_y_pos - 8, base_x_pos - 7, base_y_pos - 8, TFT_BLACK);
  M5.Lcd.fillTriangle(base_x_pos + 8, base_y_pos - 5, base_x_pos + 5, base_y_pos - 8, base_x_pos + 7, base_y_pos - 8, TFT_BLACK);
}

void drawBellIcon(uint32_t base_x_pos, uint32_t base_y_pos)
{
  M5.Lcd.fillEllipse(base_x_pos + 6, base_y_pos + 2, 2, 2, TFT_BLACK);

  M5.Lcd.fillRect(base_x_pos, base_y_pos + 3, 13, 10, TFT_BLACK);

  M5.Lcd.fillTriangle(base_x_pos, base_y_pos + 14, base_x_pos, base_y_pos + 17, base_x_pos - 4, base_y_pos + 17, TFT_BLACK);
  M5.Lcd.fillTriangle(base_x_pos + 13, base_y_pos + 14, base_x_pos + 13, base_y_pos + 17, base_x_pos + 17, base_y_pos + 17, TFT_BLACK);
  M5.Lcd.fillRect(base_x_pos, base_y_pos + 14, 13, 4, TFT_BLACK);

  M5.Lcd.fillEllipse(base_x_pos + 6, base_y_pos + 20, 2, 2, TFT_BLACK);
}

void drawClockIcon(uint32_t base_x_pos, uint32_t base_y_pos)
{
  const int32_t offset = 14;

  M5.Lcd.drawPixel(base_x_pos, base_y_pos - offset, TFT_BLACK);
  M5.Lcd.drawPixel(base_x_pos - offset, base_y_pos, TFT_BLACK);
  M5.Lcd.drawPixel(base_x_pos, base_y_pos + offset, TFT_BLACK);
  M5.Lcd.drawPixel(base_x_pos + offset, base_y_pos, TFT_BLACK);

  M5.Lcd.drawPixel(base_x_pos - 7, base_y_pos + 12, TFT_BLACK);
  M5.Lcd.drawPixel(base_x_pos - 12, base_y_pos + 7, TFT_BLACK);
  M5.Lcd.drawPixel(base_x_pos - 7, base_y_pos - 12, TFT_BLACK);
  M5.Lcd.drawPixel(base_x_pos - 12, base_y_pos - 7, TFT_BLACK);

  M5.Lcd.drawPixel(base_x_pos + 7, base_y_pos + 12, TFT_BLACK);
  M5.Lcd.drawPixel(base_x_pos + 12, base_y_pos + 7, TFT_BLACK);
  M5.Lcd.drawPixel(base_x_pos + 7, base_y_pos - 12, TFT_BLACK);
  M5.Lcd.drawPixel(base_x_pos + 12, base_y_pos - 7, TFT_BLACK);

  M5.Lcd.drawLine(base_x_pos, base_y_pos, 296, 198, TFT_BLACK);
  M5.Lcd.drawLine(base_x_pos, base_y_pos + 1, 296, 199, TFT_BLACK);
  M5.Lcd.drawLine(base_x_pos, base_y_pos, 306, 195, TFT_BLACK);
  M5.Lcd.drawLine(base_x_pos, base_y_pos + 1, 306, 196, TFT_BLACK);
}

void drawThermometerIcon(uint32_t base_x_pos, uint32_t base_y_pos, uint16_t bk_color)
{
  M5.Lcd.drawEllipse(base_x_pos, base_y_pos, 7, 7, TFT_BLACK);
  M5.Lcd.drawRoundRect(base_x_pos - 3, base_y_pos - 35, 8, 35, 4, TFT_BLACK);
  M5.Lcd.fillEllipse(base_x_pos, base_y_pos, 6, 6, bk_color);
  M5.Lcd.fillEllipse(base_x_pos, base_y_pos, 4, 4, TFT_BLACK);
  M5.Lcd.fillRoundRect(base_x_pos - 1, base_y_pos - 35 + 8, 4, 25, 2, TFT_BLACK);
}

void drawSandglassIcon(uint32_t base_x_pos, uint32_t base_y_pos, uint16_t color)
{
  M5.Lcd.drawTriangle(base_x_pos, base_y_pos, base_x_pos + 20, base_y_pos, base_x_pos + 10, base_y_pos + 14, color);
  M5.Lcd.drawTriangle(base_x_pos, base_y_pos + 28, base_x_pos + 20, base_y_pos + 28, base_x_pos + 10, base_y_pos + 14, color);

  M5.Lcd.fillTriangle(base_x_pos + 6, base_y_pos + 4, base_x_pos + 20 - 6, base_y_pos + 4, base_x_pos + 10, base_y_pos + 14 - 4, color);
  M5.Lcd.fillTriangle(base_x_pos + 2, base_y_pos + 28 - 2, base_x_pos + 20 - 2, base_y_pos + 28 - 2, base_x_pos + 10, base_y_pos + 14 + 8, color);
}

void wakeUpTime()
{
  uint16_t bkground_color = M5.Lcd.color565(255, 255, 255);
  M5.Lcd.fillScreen(bkground_color);
  M5.Lcd.setTextColor(TFT_BLACK);
  M5.Lcd.setTextSize(6);
  M5.Lcd.drawString("WAKE", 50, 100);
  M5.Lcd.drawString(" UP ", 50, 170);

  while (1)
  {
    M5.update();
    if (cl_blink_count.isHideDisplay() == false)
    {
      for(int i=0;i<NUMPIXELS;i++){
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      pixels.setPixelColor(i, pixels.Color(150,0,0)); // Moderately bright BLUE color.
      pixels.show(); // This sends the updated pixel color to the hardware.
      // delay(delayval); // Delay for a period of time (in milliseconds).
      M5.Speaker.tone(589);
      }
    }
    else
    {
      pixels.clear();
      pixels.show();
      M5.Speaker.mute();
    }
    if (M5.BtnA.wasReleased() || M5.BtnB.wasReleased() || M5.BtnC.wasReleased())
    {
      is_state_changed = true;
      e_alarm_status = ALARM_NOT_SET;
      pixels.clear();
      pixels.show();
      M5.Speaker.mute();
      cl_blink_count.resetCount();
      break;
    }
    cl_blink_count.incrementCount();
    delay(100);
  }
}

int16_t enableAlarm(uint8_t hour, uint8_t minute)
{
  int32_t set_time_minute = hour * 60 + minute;
  int32_t now_time_minute = 0;
  int32_t timer_ms = 0;
  int16_t callback_slot_id = -1;

  struct tm time_info;
  if (!getLocalTime(&time_info))
  {
    M5.Lcd.drawString("ERR", 0, 190);
  }
  else
  {
    now_time_minute = time_info.tm_hour * 60 + time_info.tm_min;
    if (set_time_minute > now_time_minute)
    {
      timer_ms = ((set_time_minute - now_time_minute) * 60 - time_info.tm_sec) * 1000;
    }
    else
    {
      timer_ms = (((24 * 60) - (now_time_minute - set_time_minute)) * 60 - time_info.tm_sec) * 1000;
    }
    callback_slot_id = Task_Timer.setTimeout(timer_ms, wakeUpTime);
  }
  return callback_slot_id;
}

void disableAlarm(int16_t callback_slot_id)
{
  if (callback_slot_id != -1)
  {
    Task_Timer.deleteTimer(callback_slot_id);
  }
}

int32_t getAngle()
{
  int32_t angle = 0;
  float accX = 0.0F;  // Define variables for storing inertial sensor data
  float accY = 0.0F;
  float accZ = 0.0F;

  M5.IMU.getAccelData(&accX, &accY,&accZ); 
  double angle_XY_direction = 0.0;
  angle_XY_direction = atan2(accY, accX);
  double angle_XY_direction_deg = angle_XY_direction * 180.0 / (M_PI);
  angle = round(angle_XY_direction_deg) + 180 + 90;
  if (angle > 360)
    {
      angle -= 360;
    }
  
  return angle;
}

#define DATE_TIME_CLOCK_REF_ANGLE (0)
#define ALARM_CLOCK_REF_ANGLE (270)
#define COUNT_DOWN_TIMER_REF_ANGLE (180)
#define THERMOMETER_REF_ANGLE (90)

#define SWITCH_APP_ANGLE_RANGE (35)
#define CURRENT_APP_ANGLE_RANGE (75)

app_state_e calcCurrentAppState(int32_t angle)
{
  app_state_e e_current_state = APP_STATE_INDEFINITE;
  static app_state_e e_prev_state = APP_STATE_DATE_TIME_CLOCK;

  switch (e_prev_state)
  {
  case APP_STATE_INDEFINITE:
  case APP_STATE_DATE_TIME_CLOCK:
  default:
    if (((DATE_TIME_CLOCK_REF_ANGLE + CURRENT_APP_ANGLE_RANGE) <= angle) && (angle < (COUNT_DOWN_TIMER_REF_ANGLE - SWITCH_APP_ANGLE_RANGE)))
    {
      e_current_state = APP_STATE_ALARM_CLOCK;
      is_state_changed = true;
    }
    else if (((COUNT_DOWN_TIMER_REF_ANGLE - SWITCH_APP_ANGLE_RANGE) <= angle) && (angle < (COUNT_DOWN_TIMER_REF_ANGLE + SWITCH_APP_ANGLE_RANGE)))
    {
      e_current_state = APP_STATE_COUNT_DOWN_TIMER;
      is_state_changed = true;
    }
    else if (((COUNT_DOWN_TIMER_REF_ANGLE + SWITCH_APP_ANGLE_RANGE) <= angle) && (angle < (360 - CURRENT_APP_ANGLE_RANGE)))
    {
      e_current_state = APP_STATE_THERMOMETER;
      is_state_changed = true;
    }
    else
    {
      e_current_state = APP_STATE_DATE_TIME_CLOCK;
    }
    break;

  case APP_STATE_THERMOMETER:
    if (((ALARM_CLOCK_REF_ANGLE + CURRENT_APP_ANGLE_RANGE) <= angle) || (angle < (THERMOMETER_REF_ANGLE - SWITCH_APP_ANGLE_RANGE)))
    {
      e_current_state = APP_STATE_DATE_TIME_CLOCK;
      is_state_changed = true;
    }
    else if (((THERMOMETER_REF_ANGLE - SWITCH_APP_ANGLE_RANGE) <= angle) && (angle < (THERMOMETER_REF_ANGLE + SWITCH_APP_ANGLE_RANGE)))
    {
      e_current_state = APP_STATE_ALARM_CLOCK;
      is_state_changed = true;
    }
    else if (((THERMOMETER_REF_ANGLE + SWITCH_APP_ANGLE_RANGE) <= angle) && (angle < (ALARM_CLOCK_REF_ANGLE - CURRENT_APP_ANGLE_RANGE)))
    {
      e_current_state = APP_STATE_COUNT_DOWN_TIMER;
      is_state_changed = true;
    }
    else
    {
      e_current_state = APP_STATE_THERMOMETER;
    }
    break;

  case APP_STATE_COUNT_DOWN_TIMER:
    if (((COUNT_DOWN_TIMER_REF_ANGLE + CURRENT_APP_ANGLE_RANGE) <= angle) && (angle < (360 - SWITCH_APP_ANGLE_RANGE)))
    {
      e_current_state = APP_STATE_THERMOMETER;
      is_state_changed = true;
    }
    else if (((360 - SWITCH_APP_ANGLE_RANGE) <= angle) || (angle < (DATE_TIME_CLOCK_REF_ANGLE + SWITCH_APP_ANGLE_RANGE)))
    {
      e_current_state = APP_STATE_DATE_TIME_CLOCK;
      is_state_changed = true;
    }
    else if (((DATE_TIME_CLOCK_REF_ANGLE + SWITCH_APP_ANGLE_RANGE) <= angle) && (angle < (COUNT_DOWN_TIMER_REF_ANGLE - CURRENT_APP_ANGLE_RANGE)))
    {
      e_current_state = APP_STATE_ALARM_CLOCK;
      is_state_changed = true;
    }
    else
    {
      e_current_state = APP_STATE_COUNT_DOWN_TIMER;
    }
    break;

  case APP_STATE_ALARM_CLOCK:
    if (((THERMOMETER_REF_ANGLE + CURRENT_APP_ANGLE_RANGE) <= angle) && (angle < (ALARM_CLOCK_REF_ANGLE - SWITCH_APP_ANGLE_RANGE)))
    {
      e_current_state = APP_STATE_COUNT_DOWN_TIMER;
      is_state_changed = true;
    }
    else if (((ALARM_CLOCK_REF_ANGLE - SWITCH_APP_ANGLE_RANGE) <= angle) && (angle < (ALARM_CLOCK_REF_ANGLE + SWITCH_APP_ANGLE_RANGE)))
    {
      e_current_state = APP_STATE_THERMOMETER;
      is_state_changed = true;
    }
    else if (((ALARM_CLOCK_REF_ANGLE + SWITCH_APP_ANGLE_RANGE) <= angle) || (angle < (THERMOMETER_REF_ANGLE - CURRENT_APP_ANGLE_RANGE)))
    {
      e_current_state = APP_STATE_DATE_TIME_CLOCK;
      is_state_changed = true;
    }
    else
    {
      e_current_state = APP_STATE_ALARM_CLOCK;
    }
    break;
  }

  e_prev_state = e_current_state;
  return e_current_state;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Incoming MQTT: ");
  Serial.println(topic);
  String messageTemp;

  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length); 
  serializeJsonPretty(doc, Serial);
  Serial.println();

  if (doc["temperature"] != nullptr) {
    temper = doc["temperature"];
    if (temper < 0) {
      temper-=0.5; //For rounding the number subtracted 0.5
    } else {
      temper+=0.5; //For rounding the number added 0.5 
    }
  }
  if (doc["brightness"] != nullptr) BRIGHTNESS = doc["brightness"];
  if (doc["powersave"] != nullptr) POWERSAVE = doc["powersave"];
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("M5stack", mqttUser, mqttPassword)) {
      Serial.println("connected");
      // Subscribe
      client.subscribe(m5ikea_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  M5.begin();
  Wire.begin();
  M5.Lcd.setBrightness(BRIGHTNESS);

  // MPU Init
  M5.IMU.Init();

  M5.Power.begin();
  M5.Power.setPowerBoostSet(true);
  M5.Power.setPowerBoostKeepOn(true); //Disables when power consumption < 45mA, IP5306 automatically goes to standby in 32 seconds.

  pixels.begin();
  pixels.show();
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    delay(100);
  }

  M5.Lcd.fillScreen(TFT_WHITE);
  cl_blink_count.resetCount();
  delay(1000);

  // init time setting
  configTime(gmt_offset_sec, daylight_offset_sec, ntp_server_1st, ntp_server_2nd);
  struct tm time_info;
  if (!getLocalTime(&time_info))
  {
    M5.Lcd.fillScreen(TFT_RED);
    delay(3000);
  }
  //MQTT Connection
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pw_save_count.resetCount();    

  Serial.println("Started M5Ikea");
}

void loop()
{
  static app_state_e e_app_state = APP_STATE_INDEFINITE;

  Bat_Level = M5.Power.getBatteryLevel();
  if (M5.Power.isCharging()) {
    isModulePluggedIn = true;
  } else {
    isModulePluggedIn = false;
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  M5.update();
  Task_Timer.run();

  int32_t body_angle = getAngle();
  if (body_angle != -1)
  {
    e_app_state = calcCurrentAppState(body_angle);
  }

  switch (e_app_state)
  {
  case APP_STATE_ALARM_CLOCK:
    M5.Lcd.setRotation(0);
    displayAlarmScreen();
    break;

  case APP_STATE_DATE_TIME_CLOCK:
    M5.Lcd.setRotation(1);
    displayDateTimeScreen();
    break;

  case APP_STATE_THERMOMETER:
    M5.Lcd.setRotation(2);
    displayThermometerScreen();
    break;

  case APP_STATE_COUNT_DOWN_TIMER:
    M5.Lcd.setRotation(3);
    displayCountdownTimerScreen();
    break;

  case APP_STATE_INDEFINITE:
  default:
    break;
  }

  if (POWERSAVE == true) {
    if (pw_save_count.isDimDisplay() == false) {
        M5.Lcd.setBrightness(10);
      } else {
        M5.Lcd.setBrightness(BRIGHTNESS);
      }
  } else {
    M5.Lcd.setBrightness(BRIGHTNESS);
  }

  cl_blink_count.incrementCount();
  pw_save_count.incrementCount();
  delay(100);
}
