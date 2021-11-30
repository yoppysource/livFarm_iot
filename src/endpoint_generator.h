#include <arduino_secrets.h>
#include <Arduino.h>

class EndpointGenerator;

class EndpointGenerator
{
  private:
  const String INIT_PATH = SECRET_PATH_INIT;
  const String PLANTER_ID = SECRET_PLANTER_ID;
  const String ROOT_PATH = SECRET_PATH_ROOT;
  const String SNAPSHOTS_PATH = "/snapshots";
  public:
  
  String getInitPath() {
    return String("/planters/") + SECRET_PLANTER_ID;
  }
  String getSnapshotsPath() {
    return SNAPSHOTS_PATH;
  }
  

};