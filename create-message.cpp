#include <span>
#include <cstddef>
#include <cstdint>

enum Command : std::uint8_t
{
	Shock = 1,
	Vibrate = 2,
	Sound = 3,

	_Min = Shock,
	_Max = Sound
};
constexpr bool CreateMessage(std::uint16_t transmitterId, std::uint8_t channel, Command command, std::uint8_t strength, std::span<std::byte, 22> message)
{
	// Parameters exceed their allocated binary size in the message
	if (channel > 0xF || command > 0xF || strength > 0xFF)
	{
		return false;
	}

	// Command is not valid
	if (command < Command::_Min || command > Command::_Max)
	{
		return false;
	}


	int checksum = 59 + command + ((channel + 1) * 16) + strength;

	if (checksum > 0xFF)
	{
		return false;
	}

	std::uint64_t buffer
		= std::uint64_t(transmitterId) << 24
		| std::uint64_t(channel & 0xF) << 20
		| std::uint64_t(command & 0xF) << 16
		| std::uint64_t(strength) << 8
		| std::uint64_t(checksum & 0xFF);

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
