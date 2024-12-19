#include <TOTP.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <Base32.h>

#define EEPROM_SIZE 512

struct Record {
  uint16_t hash;
  uint16_t id;
  uint8_t secret[16];
};

static const int MAX_RECORDS = EEPROM_SIZE / sizeof(Record);

const int rs = PB11, en = PB10, d4 = PB0, d5 = PB1, d6 = PC13, d7 = PC14;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int SERIAL_BAUD_RATE = 9600;

enum HostAPICode {
  GET_SITES = '0',
  GET_AUTH_CODE = '1',
  ADD_SITE = '2',
  REM_SITE = '3',
  SET_TIME = '4',
  ADD_RECORD = '5',
};

const int TIME_STEP = 30;

const int SIZE = 2;
const char* sites_id[SIZE]   = {"1", "2"};
const char* sites_hash[SIZE] = {"hash1", "hash2"};
const char* sites_encoded_code[SIZE] = {"code1", "2S6NHETOPYDV3K5V"};

const int ui_timer_period = 5000000;
const int totp_timer_period = 1000000;

volatile long global_current_time = 0;
volatile long ui_current_time = 0;

volatile bool button_pressed = false;
volatile bool ui_timer_updated = false;

void do_better() {
  EEPROM.format();
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  pinMode(PA0, INPUT);
  attachInterrupt(PA0, on_button_push, FALLING);

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

  // do_better();
}

void loop() {
  if (Serial.available() > 0) {
    char incoming_code = Serial.read();

    if (incoming_code == GET_SITES) {
      get_list_of_sites_id();
    } else if (incoming_code == GET_AUTH_CODE) {
      // get_site_auth_code();
      uint16_t id = 2;
      String site = "totp.danhersam.com";
      String login = "user";
      long currentTime = 0;
      getAuthCode(id, site, login, currentTime);
    } else if (incoming_code == SET_TIME) {
      set_time();
    } else if (incoming_code == ADD_RECORD) {
      add_record();
    } else {
      Serial.println("Error: HostAPICode not implemented!");
    }
  }
}

void add_record() {
  uint16_t id = 2;
  String site = "totp.danhersam.com";
  String login = "user";
  const char* encoded_code = "2S6NHETOPYDV3K5V";
  uint8_t decoded_code[16]; // Buffer to store decoded data
  int decoded_code_length = decode_base32(encoded_code, decoded_code);
  addRecord(id, site, login, decoded_code, decoded_code_length);
}

void get_list_of_sites_id() {
  String list_of_sites_id = "";

  for (int i = 0; i < SIZE; ++i) {
    list_of_sites_id += sites_id[i];
    list_of_sites_id += ",";
  }

  Serial.println(list_of_sites_id);
}

void get_site_auth_code() {
  String site_id = get_string();
  String site_name = get_string();

  char *endptr;
  long current_time = strtol(get_string().c_str(), &endptr, 10);

  if (site_id.length() == 0 || site_name.length() == 0) {
    Serial.println("Error: Invalid input!");
    return;
  }

  int i = get_index(site_id);
  if (i == -1 || !check_site_name(site_name, sites_hash[i])) {
    Serial.println("Something went wrong!");
    return;
  }

  lcd.clear();
  lcd.print( "Send TOTP code?" );
  lcd.setCursor( 0, 1 );
  lcd.print(sites_encoded_code[i]);

  Timer3.pause();
  Timer3.refresh();

  button_pressed = false;
  ui_timer_updated = false;
  ui_current_time = 0;
  
  Timer3.resume();

  while (!button_pressed && !ui_timer_updated) {
    delay(300);
  }

  lcd.clear();
  if (button_pressed) {
    lcd.print( "Confirmed" );
    String site_auth_code = get_site_auth_code(sites_hash[i], current_time);
    Serial.println(site_auth_code);
  } else {
    lcd.print( "Rejected" );
  }
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

int get_index(String site_id) {
  for (int i = 0; i < SIZE; ++i) {
    if (site_id == sites_id[i]) {
      return i;
    }
  }
  return -1;
}

int check_site_name(String site_name, String site_hash) {
  // TODO: Проверка соответствия site_name и site_hash
  return 1; 
}

String get_site_auth_code(const char* site_encoded_code, long current_time) {
  uint8_t site_decoded_code[128]; // Buffer to store decoded data
  int site_decoded_code_length = decode_base32(site_encoded_code, site_decoded_code);
  
  TOTP totp(site_decoded_code, site_decoded_code_length);

  long time_steps = current_time / TIME_STEP;
  String site_auth_code = totp.getCodeFromSteps(time_steps);

  return site_auth_code;
}

void on_button_push() {
  button_pressed = true;
}

void on_ui_timer_update() {
  ui_timer_updated = true;
  ui_current_time++;
}

void on_totp_timer_update() {
  global_current_time++;
}

void set_time() {
  char *endptr;
  long curr_time = strtol(get_string().c_str(), &endptr, 10);

  Timer4.pause();
  Timer4.refresh();

  global_current_time = curr_time;
  
  Timer3.resume();
}

uint16_t fnv1aHash(const String& site, const String& login) {
  uint32_t hash = 2166136261;
  for (char c : site + login) {
    hash ^= c;
    hash *= 16777619;
  }
  return (uint16_t)(hash & 0xFFFF);
}

bool writeRecord(uint16_t index, const Record& record) {
  if (index >= MAX_RECORDS) return false;

  uint16_t addr = index * sizeof(Record);
  for (uint16_t i = 0; i < sizeof(Record); i++) {
    EEPROM.write(addr + i, *((uint8_t*)&record + i));
  }
  return true;
}

bool readRecord(uint16_t index, Record& record) {
  if (index >= MAX_RECORDS) return false;

  uint16_t addr = index * sizeof(Record);
  for (uint16_t i = 0; i < sizeof(Record); i++) {
    *((uint8_t*)&record + i) = EEPROM.read(addr + i);
  }
  return true;
}

bool findRecordById(uint16_t id, Record& record) {
  for (uint16_t i = 0; i < MAX_RECORDS; i++) {
    Record temp;
    if (readRecord(i, temp) && temp.id == id) {
      record = temp;
      return true;
    }
  }
  return false;
}

bool validateRecordHash(const Record& record, const String& site,
                        const String& login) {
  return record.hash == fnv1aHash(site, login);
}

void addRecord(const uint16_t id, const String& site, const String& login, const uint8_t* secret, size_t secretLength) {
  if (secretLength > 16) {
    Serial.println("Error: Secret too long");
    return;
  }

  Record record;
  record.hash = fnv1aHash(site, login);
  record.id = id;
  memset(record.secret, 0, sizeof(record.secret));
  memcpy(record.secret, secret, secretLength);

  for (uint16_t i = 0; i < MAX_RECORDS; i++) {
    Record temp;
    if (!readRecord(i, temp) || temp.id == 0xFFFF) {  // Пустая запись
      if (writeRecord(i, record)) {
        Serial.println("Record added successfully");
        return;
      } else {
        Serial.println("Error writing record");
        return;
      }
    }
  }
  Serial.println("Error: EEPROM full");
}

void getAuthCode(uint16_t id, const String& site, const String& login,
                 long currentTime) {
  Record record;
  if (!findRecordById(id, record)) {
    Serial.println("Error: Record not found");
    return;
  }

//  if (!validateRecordHash(record, site, login)) {
//    Serial.println("Error: Hash mismatch");
//    return;
//  }

  TOTP totp(record.secret, sizeof(record.secret));
  long timeSteps = currentTime / TIME_STEP;
  String authCode = totp.getCodeFromSteps(timeSteps);

  Serial.println(authCode);
}
