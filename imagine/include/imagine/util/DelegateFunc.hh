#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/util/concepts.hh>
#include <imagine/util/utility.h>
#include <new>
#include <cstddef>
#include <cassert>
#include <array>

namespace IG
{

constexpr struct DelegateFuncDefaultInit{} delegateFuncDefaultInit;

template <size_t, size_t, class, class ...> class DelegateFuncBase;

template <size_t StorageSize, size_t Align, class R, class ...Args>
class DelegateFuncBase<StorageSize, Align, R(Args...)>
{
public:
	using FreeFuncPtr = R (*)(Args...);

	constexpr DelegateFuncBase() = default;

	constexpr DelegateFuncBase(std::nullptr_t) {}

	constexpr DelegateFuncBase(DelegateFuncDefaultInit):
		DelegateFuncBase{[](Args ...args){ return R(); }} {}

	template<CallableClass<R, Args...> F>
	requires (sizeof(F) <= StorageSize && Align >= std::alignment_of_v<F>)
	constexpr DelegateFuncBase(F const &funcObj) :
		exec
		{
			[](const Storage &funcObj, Args ...args) -> R
			{
				return ((F*)funcObj.data())->operator()(IG_forward(args)...);
			}
		}
	{
		// construct from lambda/function object
		new (store.data()) F(funcObj);
	}

#ifdef IG_DELEGATE_FUNC_POINTER_SUPPORT
	constexpr DelegateFuncBase(CallableFunctionPointer<R, Args...> auto const &funcObj)
		requires (StorageSize >= sizeof(void*) && Align >= sizeof(void*)):
		exec
		{
			[](const Storage &funcObj, Args ...args) -> R
			{
				return (*((FreeFuncPtr*)funcObj.data()))(IG_forward(args)...);
			}
		}
	{
		// construct from free function
		new (store.data()) FreeFuncPtr(funcObj);
	}
#endif

	explicit constexpr operator bool() const
	{
		return exec;
	}

	constexpr R operator()(auto &&...args) const
		requires ValidInvokeArgs<FreeFuncPtr, decltype(args)...>
	{
		assert(exec);
		return exec(store, IG_forward(args)...);
	}

	constexpr bool operator ==(DelegateFuncBase const&) const = default;

	constexpr R callCopy(auto &&...args) const
	{
		// Call a copy to avoid trashing captured variables
		// if delegate's function can modify the delegate
		return ({auto copy = *this; copy;})(IG_forward(args)...);
	}

	constexpr R callSafe(auto &&...args) const
	{
		if(exec)
			return this->operator()(IG_forward(args)...);
		return R();
	}

	constexpr R callCopySafe(auto &&...args) const
	{
		if(exec)
			return callCopy(IG_forward(args)...);
		return R();
	}

private:
	using Storage = std::array<unsigned char, StorageSize>;

	alignas(Align) Storage store{};
	R (*exec)(const Storage &, Args...){};
};

template <size_t StorageSize, size_t Align, class R, class ...Args>
using DelegateFuncA = DelegateFuncBase<StorageSize, Align, R, Args...>;

template <size_t StorageSize, class R, class ...Args>
using DelegateFuncS = DelegateFuncBase<StorageSize, sizeof(void*), R, Args...>;

template <class R, class ...Args>
using DelegateFunc = DelegateFuncBase<sizeof(void*)*2, sizeof(void*), R, Args...>;

}
