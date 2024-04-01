/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/Cheats.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/gui/TextEntry.hh>

namespace EmuEx
{

BaseCheatsView::BaseCheatsView(ViewAttachParams attach):
	TableView
	{
		"Cheats",
		attach,
		[this](const TableView &)
		{
			return 1 + cheat.size();
		},
		[this](const TableView &, size_t idx) -> MenuItem&
		{
			if(idx == 0)
				return edit;
			else
				return cheat[idx - 1];
		}
	},
	edit
	{
		"Add/Edit", attach,
		[this](const Input::Event &e)
		{
			auto editCheatsView = app().makeView(attachParams(), EmuApp::ViewID::EDIT_CHEATS);
			static_cast<BaseEditCheatListView*>(editCheatsView.get())->setOnCheatListChanged(
				[this]()
				{
					auto selectedCell = selected;
					loadCheatItems();
					highlightCell(selectedCell);
					place();
				});
			pushAndShow(std::move(editCheatsView), e);
		}
	} {}

BaseEditCheatListView::BaseEditCheatListView(ViewAttachParams attach, TableView::ItemsDelegate items, TableView::ItemDelegate item):
	TableView
	{
		"Edit Cheats",
		attach,
		items,
		item
	} {}

void BaseEditCheatListView::setOnCheatListChanged(RefreshCheatsDelegate del)
{
	onCheatListChanged_ = del;
}

void BaseEditCheatListView::onCheatListChanged()
{
	auto selectedCell = selected;
	loadCheatItems();
	highlightCell(selectedCell);
	place();
	onCheatListChanged_.callSafe();
}

}
