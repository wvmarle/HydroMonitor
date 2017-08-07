#include <HydroMonitorCore.h>

HydroMonitorCore::HydroMonitorCore () {
}

void HydroMonitorCore::setLogging(LogFunction f) {
  writeLog = f;
}

void HydroMonitorCore::setLoglevel(int lvl) {
  loglevel = lvl;
}

