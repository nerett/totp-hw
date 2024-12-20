#include <TOTP.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <Base32.h>

#define EEPROM_SIZE 512

struct Site {
  uint16_t id;
  uint16_t hash;
  uint8_t decoded_code[16];
};

static const int MAX_SITES = EEPROM_SIZE / sizeof(Site);

const int rs = PB11, en = PB10, d4 = PB0, d5 = PB1, d6 = PC13, d7 = PC14;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int SERIAL_BAUD_RATE = 9600;

enum HostAPICode {
  SET_TIME = '1',
  ADD_SITE = '2',
  GET_OTP = '3',
  ERASE_DB = '4',
};

const int TIME_STEP = 30;

const int ui_timer_period = 1000000;
const int totp_timer_period = 1000000;

const int ui_timer_threshold = 5;
const int ui_timer_clear_screen_threshold = 8;

volatile long global_current_time = 0;
volatile long ui_current_time = 0;
volatile bool do_ui_clear_screen = false;

volatile bool button_pressed = false;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  pinMode(PA0, INPUT);
  attachInterrupt(PA0, on_button_push, FALLING);

  pinMode( PA1, OUTPUT );
  digitalWrite(PA1, LOW);

  Timer3.pause();
  Timer3.setPeriod(ui_timer_period);
  Timer3.attachInterrupt(TIMER_UPDATE_INTERRUPT, on_ui_timer_update);

  Timer4.pause();
  Timer4.setPeriod(totp_timer_period);
  Timer4.attachInterrupt(TIMER_UPDATE_INTERRUPT, on_totp_timer_update);

  lcd.begin( 16, 2 );
  lcd.print( "HW TOTP token" );
  lcd.setCursor( 0, 1 );
  lcd.print( "MIPT, 2025" );

  reinit_ui_wait();
}

void loop() {
  if (Serial.available() > 0) {
    char incoming_code = Serial.read();
    
    if (incoming_code == SET_TIME) {
      set_time();
    } else if (incoming_code == ADD_SITE) {
      add_site();
    } else if (incoming_code == GET_OTP) {
      get_otp();
    } else if (incoming_code == ERASE_DB) {
      erase_db();
    } else {
      Serial.println("Error: HostAPICode not implemented!");
    }
  }

  if (do_ui_clear_screen) {
    lcd.clear();
    lcd.print("Awaiting command");
  }

  delay(100);
}

void set_time() {
  char *endptr;
  long curr_time = strtol(get_string().c_str(), &endptr, 10);

  Timer4.pause();
  Timer4.refresh();

  global_current_time = curr_time;
  
  Timer4.resume();
}

void add_site() {
  uint16_t site_id = (uint16_t) strtoul(get_string().c_str(), NULL, 10);
  String site_name = get_string();
  String site_login = get_string();

  const char* site_encoded_code = get_string().c_str();
  uint8_t site_decoded_code[16];
  int site_decoded_code_length = decode_base32(site_encoded_code, site_decoded_code);

  String state = add_site(site_id, site_name, site_login, site_decoded_code, site_decoded_code_length);
  Serial.print(state);
}

String add_site(const uint16_t site_id, const String& site_name, const String& site_login, const uint8_t* site_decoded_code, const int site_decoded_code_length) {
  if (site_decoded_code_length > 16) {
    return "Error: Secret too long";
  }

  Site site;
  site.hash = fnv1a_hash(site_name, site_login);
  site.id = site_id;
  memset(site.decoded_code, 0, sizeof(site.decoded_code));
  memcpy(site.decoded_code, site_decoded_code, site_decoded_code_length);

  for (uint16_t i = 0; i < MAX_SITES; ++i) {
    Site temp;
    if (!read_site(i, temp) || temp.id == 0xFFFF) { // Пустая запись
      if (write_site(i, site)) {
        return "Site added successfully";
      } else {
        return "Error writing site";
      }
    }
  }
  
  return "Error: EEPROM full";
}

bool read_site(const uint16_t index, const Site& site) {
  if (index >= MAX_SITES) {
    return false;
  }

  uint16_t addr = index * sizeof(Site);
  for (uint16_t i = 0; i < sizeof(Site); ++i) {
    *((uint8_t*)&site + i) = EEPROM.read(addr + i);
  }
  
  return true;
}

bool write_site(const uint16_t index, const Site& site) {
  if (index >= MAX_SITES) {
    return false;
  }

  uint16_t addr = index * sizeof(Site);
  for (uint16_t i = 0; i < sizeof(Site); ++i) {
    EEPROM.write(addr + i, *((uint8_t*)&site + i));
  }
  
  return true;
}

void get_otp() { 
  uint16_t site_id = (uint16_t) strtoul(get_string().c_str(), NULL, 10);
  String site_name = get_string();
  String site_login = get_string();

  if (site_name.length() == 0 || site_login.length() == 0) {
    Serial.println("Error: Invalid input!");
    return;
  }

  if (global_current_time <= 0) {
    Serial.println("Error: Set time first!");
    return;
  }

  Serial.println(get_otp(site_id, site_name, site_login, global_current_time));
}

String get_otp(uint16_t site_id, const String& site_name, const String& site_login, long current_time) {
  Site site;
  if (!find_site_by_id(site_id, site)) {
    return "Error: Site not found";
  }

  if (!validate_site_hash(site, site_name, site_login)) {
    return "Error: Hash mismatch";
  }

  
  if (lcd_prompt_user("Send TOTP code?", site_name + "$" + site_login)) {
    TOTP totp(site.decoded_code, sizeof(site.decoded_code));
    long timeSteps = current_time / TIME_STEP;
    return totp.getCodeFromSteps(timeSteps);
  }

  return "Reject from user";
}

bool find_site_by_id(const uint16_t site_id, Site& site) {
  for (uint16_t i = 0; i < MAX_SITES; ++i) {
    Site temp;
    if (read_site(i, temp) && temp.id == site_id) {
      site = temp;
      return true;
    }
  }
  return false;
}

bool validate_site_hash(const Site& site, const String& site_name, const String& site_login) {
  return site.hash == fnv1a_hash(site_name, site_login);
}

uint16_t fnv1a_hash(const String& site_name, const String& site_login) {
  uint32_t hash = 2166136261;
  for (char c : site_name + site_login) {
    hash ^= c;
    hash *= 16777619;
  }
  return (uint16_t)(hash & 0xFFFF);
}

void erase_db() {
  if (lcd_prompt_user("Erase TOTP", "token database?")) {
    EEPROM.format();
  }
}

void reinit_ui_wait() {
  Timer3.pause();
  
  button_pressed = false;
  do_ui_clear_screen = false;
  ui_current_time = 0;
  
  Timer3.refresh();
  Timer3.resume();
}

bool lcd_prompt_user(String line1, String line2) {
  line2 = " " + line2 + " ";
  
  lcd.clear();
  lcd.print(line1);
  digitalWrite(PA1, HIGH);

  reinit_ui_wait();
  int i = 0;
  while (!button_pressed && ui_current_time < ui_timer_threshold) {
    if (i >= line2.length()) {
      i = 0;
    }
    lcd.setCursor(0, 1);
    lcd.print(line2.substring(++i));
    
    delay(400);
  }

  lcd.clear();
  digitalWrite(PA1, LOW);
  if (button_pressed) {
    lcd.print( "V Confirmed" );
    return true;
  }
  
  lcd.print( "X Rejected" );
  return false;
}

String get_string() {
  String string = "";
  
  while (true) {
    if (Serial.available() > 0) {
      char received_char = Serial.read();
      if (received_char == ' ' || received_char == '\n') {
        break;
      } else {
        string += received_char;
      }
    }
    delay(10);
  }
  
  return string;
}

void on_button_push() {
  button_pressed = true;
}

void on_ui_timer_update() {
  ui_current_time++;
  if (ui_current_time > ui_timer_clear_screen_threshold) {
    do_ui_clear_screen = true;
  }
}

void on_totp_timer_update() {
  global_current_time++;
}
