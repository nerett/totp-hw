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
static const int SERIAL_BAUD_RATE = 9600;  // Скорость передачи данных бит/с

// Хэш-функция: FNV-1a
uint16_t fnv1aHash(const String& site, const String& login) {
  uint32_t hash = 2166136261;
  for (char c : site + login) {
    hash ^= c;
    hash *= 16777619;
  }
  return (uint16_t)(hash & 0xFFFF);
}

// Запись данных в EEPROM
bool writeRecord(uint16_t index, const Record& record) {
  if (index >= MAX_RECORDS) return false;

  uint16_t addr = index * sizeof(Record);
  for (uint16_t i = 0; i < sizeof(Record); i++) {
    EEPROM.write(addr + i, *((uint8_t*)&record + i));
  }
  return true;
}

// Чтение данных из EEPROM
bool readRecord(uint16_t index, Record& record) {
  if (index >= MAX_RECORDS) return false;

  uint16_t addr = index * sizeof(Record);
  for (uint16_t i = 0; i < sizeof(Record); i++) {
    *((uint8_t*)&record + i) = EEPROM.read(addr + i);
  }
  return true;
}

// Поиск записи по ID
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

// Проверка записи по хэшу
bool validateRecordHash(const Record& record, const String& site,
                        const String& login) {
  return record.hash == fnv1aHash(site, login);
}

// Добавление записи в EEPROM
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

void example() {
  Serial.begin(SERIAL_BAUD_RATE);
  EEPROM.init();  // Инициализация EEPROM

  // Пример добавления записи
  uint8_t secret[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
  addRecord(1, "example.com", "user", secret, sizeof(secret));

  // Пример получения одноразового пароля
  getAuthCode(1, "example.com", "user", 123456);
}
