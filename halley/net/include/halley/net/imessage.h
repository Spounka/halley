#pragma once
#include <gsl/gsl>

namespace Halley
{
	class IMessage
	{
	public:
		virtual ~IMessage() {}

		virtual size_t getSerializedSize() = 0;
		virtual void serializeTo(gsl::span<gsl::byte> dst) = 0;
	};
}
