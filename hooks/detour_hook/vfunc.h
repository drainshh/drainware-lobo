#pragma once

#include <cstddef>

namespace VirtualMethod
{
	template< typename T, std::size_t Idx, typename... Args >
	constexpr T call( void* classBase, Args... args ) noexcept
	{
		return ( *reinterpret_cast< T( __thiscall*** )( void*, Args... ) >( classBase ) )[ Idx ]( classBase, args... );
	}
} // namespace VirtualMethod

#define VIRTUAL_METHOD( returnType, name, idx, args, argsRaw )                                                                                       \
	returnType name args noexcept                                                                                                                    \
	{                                                                                                                                                \
		return VirtualMethod::call< returnType, idx > argsRaw;                                                                                       \
	}

#define VIRTUAL_METHOD_V( returnType, name, idx, args, argsRaw ) VIRTUAL_METHOD( returnType, name, idx, args, argsRaw )
