#include <EEPROM.h>
#include <TOTP.h>

#define EEPROM_SIZE 512

// Структура записи для хранения данных в EEPROM
struct Record {
  uint16_t hash;
  uint16_t id;
  uint8_t secret[16];
};

static const int MAX_RECORDS = EEPROM_SIZE / sizeof(Record);

static const int TIME_STEP = 30;  // Время действия одноразового пароля
static int SERIAL_BAUD_RATE = 9600;  // Скорость передачи данных данных бит/с

uint16_t fnv1aHash(const String& site, const String& login) {
  uint32_t hash = 2166136261;
  for (char c : site + login) {
    hash ^= c;
    hash *= 16777619;
  }
  return (uint16_t)(hash & 0xFFFF);
}

bool writeRecord(uint16_t id, const Record& record) {
  if (id >= MAX_RECORDS) return false;

  uint16_t addr = id * sizeof(Record);
  EEPROM.put(addr, record);
  EEPROM.commit();
  return true;
}

bool readRecord(uint16_t id, Record& record) {
  if (id >= MAX_RECORDS) return false;

  uint16_t addr = id * sizeof(Record);
  EEPROM.get(addr, record);
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

// Функция проверки записи по хэшу
bool validateRecordHash(const Record& record, const String& site,
                        const String& login) {
  return record.hash == fnv1aHash(site, login);
}

void addRecord(uint16_t id, const String& site, const String& login,
               const uint8_t* secret, size_t secretLength) {
  if (secretLength > 16) {
    Serial.println("Error: Secret too long");
    return;
  }

  Record record;
  record.hash = fnv1aHash(site, login);
  record.id = id;
  memset(record.secret, 0, sizeof(record.secret));
  memcpy(record.secret, secret, secretLength);

  if (!writeRecord(id, record)) Serial.println("Error writing record");
}

// Получение одноразового пароля. Проверяет наличие записи,
// валидирует хэш, генерирует TOTP.
void getAuthCode(uint16_t id, const String& site, const String& login,
                 long currentTime) {
  Record record;
  if (!findRecordById(id, record)) {
    Serial.println("Error: Record not found");
    return;
  }

  if (!validateRecordHash(record, site, login)) {
    Serial.println("Error: Hash mismatch");
    return;
  }

  TOTP totp(record.secret, sizeof(record.secret));
  long timeSteps = currentTime / TIME_STEP;
  String authCode = totp.getCodeFromSteps(timeSteps);

  Serial.println(authCode);
}

// Пример. Можете удалить или переименовать
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  EEPROM.begin(EEPROM_SIZE);

  // Пример добавления записи
  uint8_t secret[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
  addRecord(1, "example.com", "user", secret, sizeof(secret));

  // Пример получения одноразового пароля
  getAuthCode(1, "example.com", "user", 123456);
}
