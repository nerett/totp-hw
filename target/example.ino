 const int GET_LIST_OF_SITES = 48;
const int GET_CODE_FOR_SITE = 49;

const char* sites = "1,3,";

int incomingByte = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  if (Serial.available() > 0) {
    incomingByte = Serial.read();

    if (incomingByte == GET_LIST_OF_SITES) {
      get_list_of_sites();
    }

    if (incomingByte == GET_CODE_FOR_SITE) {
      get_code_for_site();
    }
  }

  delay(500);
}

void get_list_of_sites() {
  Serial.println(sites);
}

void get_code_for_site() {

}
