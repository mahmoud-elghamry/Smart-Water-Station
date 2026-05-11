#ifndef AI_SERVICE_H
#define AI_SERVICE_H

#include "AppTypes.h"

class AiService
{
public:
  void begin();
  AiResult run(const TelemetrySnapshot &snapshot);
};

#endif // AI_SERVICE_H
