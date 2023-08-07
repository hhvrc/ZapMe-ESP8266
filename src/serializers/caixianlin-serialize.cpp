#include "caixianlin-serialize.hpp"

constexpr std::uint16_t Checksum8(std::uint16_t value)
{
	return (value >> 8) + (value & 0xFF);
}

template <std::size_t N, std::integral T>
constexpr void FillEncodedBits(std::span<std::byte, N> buffer, std::size_t bufferOffset, T value, std::size_t valueOffset = 0)
{
	std::int64_t valueBits = (sizeof(T) * 8) - valueOffset;
	std::int64_t bufferSize = N - bufferOffset;

	if (valueBits <= 0 || bufferSize <= 0 || (bufferSize * 2) < valueBits)
	{
		return;
	}

	std::size_t bufferLastIndex = bufferOffset + (valueBits / 2) - 1;
	if (valueOffset & 1)
	{
		buffer[bufferLastIndex--] = static_cast<std::byte>(0x80 | ((value & 1) * 0x60));
		value >>= 1;
	}

	while (bufferLastIndex >= bufferOffset)
	{
		buffer[bufferLastIndex--] = static_cast<std::byte>(0x88 | ((value & 2) * 0x30) | ((value & 1) * 0x06));
		value >>= 2;
	}
}

constexpr bool CreateMessage(std::uint16_t transmitterId, Channel channel, Command command, std::uint8_t strength, std::span<std::byte, 22> message)
{
	if (channel < Channel::_Min || channel > Channel::_Max || command < Command::_Min || command > Command::_Max || strength > 99)
	{
		return false;
	}

	std::uint8_t channelValue = static_cast<std::uint8_t>(channel);
	std::uint8_t commandValue = static_cast<std::uint8_t>(command);

	std::uint8_t checksum = Checksum8(transmitterId) + channelValue + commandValue + strength;

	message[0] = std::byte{ 0xFC };
	FillEncodedBits(message, 1, transmitterId);
	FillEncodedBits(message, 9, channelValue, 4);
	FillEncodedBits(message, 11, commandValue, 4);
	FillEncodedBits(message, 13, strength);
	FillEncodedBits(message, 17, checksum);
	message[21] = std::byte{ 0x88 };

	return true;
}
