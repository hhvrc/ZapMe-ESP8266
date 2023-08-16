#pragma once

#include <cstdint>

struct WebServices {
  static void Start();
  static void Stop();
  static bool IsRunning();
  static void Update();
};
