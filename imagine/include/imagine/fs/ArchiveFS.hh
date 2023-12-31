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

#include <imagine/config/defs.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/io/ArchiveIO.hh>
#include <imagine/util/string/CStringView.hh>
#include <memory>
#include <iterator>
#include <string_view>
#include <concepts>

namespace IG
{
class IO;
}

namespace IG::FS
{

class ArchiveIterator
{
public:
	using iterator_category = std::input_iterator_tag;
	using value_type = ArchiveIO;
	using difference_type = ptrdiff_t;
	using pointer = value_type*;
	using reference = value_type&;
	struct Sentinel {};

	constexpr ArchiveIterator() = default;
	ArchiveIterator(CStringView path);
	ArchiveIterator(IO);
	ArchiveIterator(FileIO);
	ArchiveIterator(ArchiveIO);
	ArchiveIterator(const ArchiveIterator&) = default;
	ArchiveIterator(ArchiveIterator&&) = default;
	ArchiveIterator &operator=(ArchiveIterator &&o) = default;
	ArchiveIO& operator*();
	ArchiveIO* operator->();
	void operator++();
	bool operator==(Sentinel) const { return !hasEntry(); }
	void rewind();
	bool hasEntry() const { return impl.get() && impl->hasEntry(); }
	bool hasArchive() const { return impl.get() && impl->hasArchive(); }

private:
	std::shared_ptr<ArchiveIO> impl;
};

static const auto &begin(const ArchiveIterator &iter)
{
	return iter;
}

static auto end(const ArchiveIterator &)
{
	return ArchiveIterator::Sentinel{};
}

inline bool seekInArchive(ArchiveIterator &it, std::predicate<const ArchiveIO &> auto &&pred)
{
	for(ArchiveIO &entry : it)
	{
		if(pred(entry))
		{
			return true;
		}
	}
	return false;
}

inline ArchiveIO findInArchive(ArchiveIterator it, std::predicate<const ArchiveIO &> auto &&pred)
{
	if(seekInArchive(it, pred))
		return std::move(*it);
	else
		return {};
}

inline ArchiveIO findFileInArchive(ArchiveIterator it, std::predicate<const ArchiveIO &> auto &&pred)
{
	return findInArchive(it, [&](const ArchiveIO &entry){ return entry.type() == file_type::regular && pred(entry); });
}

inline ArchiveIO findFileInArchive(ArchiveIterator it, std::string_view path)
{
	return findFileInArchive(it, [&](const ArchiveIO &entry){ return entry.name() == path; });
}

inline ArchiveIO findDirectoryInArchive(ArchiveIterator it, std::predicate<const ArchiveIO &> auto &&pred)
{
	return findInArchive(it, [&](const ArchiveIO &entry){ return entry.type() == file_type::directory && pred(entry); });
}

inline bool seekFileInArchive(ArchiveIterator &it, std::predicate<const ArchiveIO &> auto &&pred)
{
	return seekInArchive(it, [&](const ArchiveIO &entry){ return entry.type() == file_type::regular && pred(entry); });
}

inline bool seekFileInArchive(ArchiveIterator &it, std::string_view path)
{
	return seekFileInArchive(it, [&](const ArchiveIO &entry){ return entry.name() == path; });
}

bool hasArchiveExtension(std::string_view name);

};
