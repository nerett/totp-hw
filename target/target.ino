const char GET_LIST_OF_SITES_ID = '0';
const char GET_SITE_AUTH_CODE = '1';

const int size = 4;
const char* sites_id[size]   = {"1", "2", "3", "4"};
const char* sites_hash[size] = {"hash1", "hash2", "hash3", "hash4"};
const char* sites_code[size] = {"code1", "code2", "code3", "code4"};

char incoming_byte = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  if (Serial.available() > 0) {
    incoming_byte = Serial.read();

    if (incoming_byte == GET_LIST_OF_SITES_ID) {
      get_list_of_sites_id();
    } else if (incoming_byte == GET_SITE_AUTH_CODE) {
      get_site_auth_code();
    }
  }
}

void get_list_of_sites_id() {
  String list_of_sites_id = "";

  for (int i = 0; i < size; ++i) {
    list_of_sites_id += sites_id[i];
    list_of_sites_id += ",";
  }

  Serial.println(list_of_sites_id);
}

void get_site_auth_code() {
  String site_id = get_string();
  String site_name = get_string();

  if (site_id.length() == 0 || site_name.length() == 0) {
    Serial.println("Error: Invalid input!");
    return;
  }

  int i = get_index(site_id);
  if (i == -1 || !check_site_name(site_name, sites_hash[i])) {
    Serial.println("Something went wrong!");
    return;
  }

  String site_auth_code = get_site_auth_code(sites_code[i]);
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
  for (int i = 0; i < size; ++i) {
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

String get_site_auth_code(String site_code) {
  // TODO: Генерация кода аутентификации
  return site_code;
}
