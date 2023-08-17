#include <cstddef>
#include <cstdint>
#include <nonstd/span.hpp>

enum class Channel : int {
  Channel1 = 0,  // Will shift the bit out og the integer, making it 0
  Channel2 = 1,
  Channel3 = 2,

  _Min = Channel1,
  _Max = Channel3
};

enum class Command : std::uint8_t {
  Shock   = 1,
  Vibrate = 2,
  Beep    = 3,

  _Min = Shock,
  _Max = Beep
};

constexpr bool CreateMessage(std::uint16_t transmitterId,
                             Channel channel,
                             Command command,
                             std::uint8_t strength,
                             nonstd::span<std::byte, 22> message);
