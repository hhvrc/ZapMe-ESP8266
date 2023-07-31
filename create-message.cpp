#include <span>
#include <cstddef>
#include <cstdint>

enum class Channel : int
{
	Channel1 = 0, // Will shift the bit out og the integer, making it 0
	Channel2 = 1,
	Channel3 = 2,

	_Min = Channel1,
	_Max = Channel3
};

enum class Command : std::uint8_t
{
	Shock = 1,
	Vibrate = 2,
	Beep = 3,

	_Min = Shock,
	_Max = Beep
};

constexpr std::uint8_t CalculateChecksum(std::uint64_t buffer)
{
	std::uint8_t checksum = 0;
	for (std::size_t i = 0; i < 8; ++i)
	{
		checksum += static_cast<std::uint8_t>(buffer & 0xFF);
		buffer >>= 8;
	}
	return checksum;
}

constexpr bool CreateMessage(std::uint16_t transmitterId, Channel channel, Command command, std::uint8_t strength, std::span<std::byte, 22> message)
{
	// Parameters exceed their allocated binary size in the message
	if (channel < Channel::_Min || channel > Channel::_Max || (std::uint8_t)command > 0xF)
	{
		return false;
	}

	// Command is not valid
	if (command < Command::_Min || command > Command::_Max)
	{
		return false;
	}

	// Strength is not valid
	if (strength > 99)
	{
		return false;
	}

	std::uint64_t buffer
		= (std::uint64_t)transmitterId << 24
		| ((std::uint64_t)channel & 0xF) << 20
		| ((std::uint64_t)command & 0xF) << 16
		| (std::uint64_t)strength << 8;

	buffer |= (std::uint64_t)CalculateChecksum(buffer);

	message[21] = std::byte{ 0x88 };
	std::size_t i = 20;
	do
	{
		message[i--] = std::byte{ 0x88 | ((buffer & 2) * 0x30) | ((buffer & 1) * 0x06) };
		buffer >>= 2;
	} while (i != 0);
	message[0] = std::byte{ 0xFC };

	return true;
}
