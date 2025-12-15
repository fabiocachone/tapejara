#pragma once
// StorageService.h
// Monta SPIFFS e persiste configurações/calibração em arquivo na flash.
// Arquivo: /config.txt (formato chave=valor, uma por linha)

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include "AppConfig.h"

struct CalibrationData {
  bool calibrated = false;

  int trimYaw   = 0;
  int trimPitch = 0;
  int trimRoll  = 0;

  int thrMin = 0;     // sugestão: 0..1000 (ou a escala que você usar)
  int thrMax = 1000;  // sugestão: 0..1000
};

class StorageService {
public:
  // Monta SPIFFS e carrega config (se não existir, cria default)
  bool begin(bool formatOnFail = true) {
    if (!SPIFFS.begin(formatOnFail)) return false;
    return loadOrCreateDefaults();
  }

  const CalibrationData& getCal() const { return cal; }
  CalibrationData& editCal() { return cal; }

  // Persiste o estado atual de cal em flash
  bool save() { return saveFile(); }

  // Recarrega do arquivo (debug)
  bool reload() { return loadFile(); }

private:
  CalibrationData cal;

  bool loadOrCreateDefaults() {
    if (!SPIFFS.exists(CONFIG_PATH)) {
      cal = CalibrationData(); // defaults
      return saveFile();
    }
    return loadFile();
  }

  bool loadFile() {
    File f = SPIFFS.open(CONFIG_PATH, "r");
    if (!f) return false;

    cal = CalibrationData(); // defaults seguros antes de ler

    while (f.available()) {
      String line = f.readStringUntil('\n');
      line.trim();
      if (line.isEmpty()) continue;

      int eq = line.indexOf('=');
      if (eq <= 0) continue;

      String key = line.substring(0, eq);
      String val = line.substring(eq + 1);
      key.trim();
      val.trim();

      if (key == "calibrated") cal.calibrated = (val == "1" || val.equalsIgnoreCase("true"));
      else if (key == "trimYaw") cal.trimYaw = val.toInt();
      else if (key == "trimPitch") cal.trimPitch = val.toInt();
      else if (key == "trimRoll") cal.trimRoll = val.toInt();
      else if (key == "thrMin") cal.thrMin = val.toInt();
      else if (key == "thrMax") cal.thrMax = val.toInt();
    }

    f.close();
    return true;
  }

  bool saveFile() {
    File f = SPIFFS.open(CONFIG_PATH, "w");
    if (!f) return false;

    f.print("calibrated="); f.println(cal.calibrated ? "1" : "0");
    f.print("trimYaw=");    f.println(cal.trimYaw);
    f.print("trimPitch=");  f.println(cal.trimPitch);
    f.print("trimRoll=");   f.println(cal.trimRoll);
    f.print("thrMin=");     f.println(cal.thrMin);
    f.print("thrMax=");     f.println(cal.thrMax);

    f.close();
    return true;
  }
};
