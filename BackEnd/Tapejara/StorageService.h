#pragma once
// StorageService.h
// Monta SPIFFS e persiste configurações simples (ex.: flag de calibração)
// em um arquivo na flash, disponível na próxima sessão.

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include "AppConfig.h"

class StorageService {
public:
  // Monta SPIFFS e carrega config (se não existir, cria default)
  bool begin(bool formatOnFail = true) {
    if (!SPIFFS.begin(formatOnFail)) {
      return false;
    }
    return loadOrCreateDefaults();
  }

  bool isCalibrated() const { return calibrated; }

  // Atualiza flag e persiste em flash
  bool setCalibrated(bool v) {
    calibrated = v;
    return save();
  }

  // Recarrega do arquivo (útil para debug)
  bool reload() {
    return load();
  }

private:
  bool calibrated = false;

  bool loadOrCreateDefaults() {
    if (!SPIFFS.exists(CONFIG_PATH)) {
      calibrated = false;
      return save(); // cria arquivo default
    }
    return load();
  }

  bool load() {
    File f = SPIFFS.open(CONFIG_PATH, "r");
    if (!f) return false;

    calibrated = false; // default seguro

    while (f.available()) {
      String line = f.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;

      int eq = line.indexOf('=');
      if (eq <= 0) continue;

      String key = line.substring(0, eq);
      String val = line.substring(eq + 1);
      key.trim();
      val.trim();

      if (key == "calibrated") {
        calibrated = (val == "1" || val.equalsIgnoreCase("true"));
      }
    }

    f.close();
    return true;
  }

  bool save() {
    File f = SPIFFS.open(CONFIG_PATH, "w");
    if (!f) return false;

    // Formato simples chave=valor
    f.print("calibrated=");
    f.println(calibrated ? "1" : "0");

    f.close();
    return true;
  }
};
