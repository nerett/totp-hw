#include <TOTP.h>
#include <Base32.h>
#include <LiquidCrystal.h>

const int rs = PB11, en = PB10, d4 = PB0, d5 = PB1, d6 = PC13, d7 = PC14;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int SERIAL_BAUD_RATE = 9600;

enum class HostAPICode = {
  GET_SITES = '0',
  GET_AUTH_CODE = '1',
  ADD_SITE = '2',
  REM_SITE = '3',
  SET_TIME = '4'
};

// const char GET_LIST_OF_SITES_ID = '0';
// const char GET_SITE_AUTH_CODE = '1';

const int TIME_STEP = 30;

const int SIZE = 2;
const char* sites_id[SIZE]   = {"1", "2"};
const char* sites_hash[SIZE] = {"hash1", "hash2"};
const char* sites_encoded_code[SIZE] = {"code1", "2S6NHETOPYDV3K5V"};

const int ui_timer_period = 5000000000;
const int totp_timer_period = 1000000000;

volatile long global_current_time = 0;

volatile bool button_pressed = false;
volatile bool ui_timer_updated = false;

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
}

void loop() {
  if (Serial.available() > 0) {
    char incoming_byte = Serial.read();

    if (incoming_byte == HostAPICode::GET_SITES) {
      get_list_of_sites_id();
    } else if (incoming_byte == HostAPICode::GET_AUTH_CODE) {
      get_site_auth_code();
    } else if (incoming_byte == HostAPICode::SET_TIME) {
      set_time();
    } else {
      Serial.println("Error: HostAPICode not implemented!");
    }
  }
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

  String site_auth_code = get_site_auth_code(sites_encoded_code[i], current_time);
  Serial.println(site_auth_code);
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
}

void on_totp_timer_update() {
  global_current_time++;
}

set_time() {
  char *endptr;
  long curr_time = strtol(get_string().c_str(), &endptr, 10);

  Timer4.pause();
  Timer4.refresh();

  global_current_time = curr_time;
  
  Timer3.resume();
}
