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

module;
#include <imagine/util/macros.h>

export module imagine.internal.io;
export import imagine;

export namespace IG
{

inline auto asString(IOAccessHint access)
{
	switch(access)
	{
		case IOAccessHint::Normal: return "Normal";
		case IOAccessHint::Sequential: return "Sequential";
		case IOAccessHint::Random: return "Random";
		case IOAccessHint::All: return "All";
	}
	bug_unreachable("IOAccessHint == %d", (int)access);
}

inline auto asString(IOAdvice advice)
{
	switch(advice)
	{
		case IOAdvice::Normal: return "Normal";
		case IOAdvice::Sequential: return "Sequential";
		case IOAdvice::Random: return "Random";
		case IOAdvice::WillNeed: return "Will Need";
	}
	bug_unreachable("IOAdvice == %d", (int)advice);
}

}
