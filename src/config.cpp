#include "config.hpp"

#include <EEPROM.h>

void ConfigDeleter(const Config* config) {
  EEPROM.end();
}

void Config::Migrate() {
  auto config = Config::LoadReadWrite();
  if (memcmp(config->__IDENTIFIER, "ZAPM", 4) != 0)
  {
    memcpy(config->__IDENTIFIER, "ZAPM", 4);
    config->__VERSION = Config::VERSION;

    strcpy(config->wifi.apName, "ZapMe");
    strcpy(config->wifi.apPassword, "12345678");

    for (std::size_t i = 0; i < 4; i++) {
      config->wifi.networks[i].flags = 0;
      memset(config->wifi.networks[i].ssid, 0, sizeof(config->wifi.networks[i].ssid));
      memset(config->wifi.networks[i].password, 0, sizeof(config->wifi.networks[i].password));
    }
  }

  if (config->__VERSION != Config::VERSION) {
    // TODO: Implement migration logic
  }
}

std::shared_ptr<const Config> Config::LoadReadOnly() {
  EEPROM.begin(sizeof(Config));

  const Config* config = reinterpret_cast<const Config*>(EEPROM.getDataPtr());

  return std::shared_ptr<const Config>(config, ConfigDeleter);
}

std::shared_ptr<Config> Config::LoadReadWrite() {
  EEPROM.begin(sizeof(Config));

  std::uint8_t* data = EEPROM.getDataPtr();

  Config* config = reinterpret_cast<Config*>(data);

  return std::shared_ptr<Config>(config, ConfigDeleter);
}

bool Config::Save() {
  return EEPROM.commit();
}
