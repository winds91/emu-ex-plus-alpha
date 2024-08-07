/* Mednafen - Multi-system Emulator
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

namespace MDFN_IEN_PCE_FAST
{

uint32 HuC_Load(Stream* fp) MDFN_COLD;
void HuC_LoadCD(const std::string& bios_path) MDFN_COLD;
void HuC_SaveNV(void) MDFN_COLD;
void HuC_LoadNV(void) MDFN_COLD;
void HuC_Kill(void) MDFN_COLD;

void HuC_StateAction(StateMem *sm, int load, int data_only);

void HuC_Power(void) MDFN_COLD;

DECLFR(PCE_ACRead);
DECLFW(PCE_ACWrite);

MDFN_HIDE extern bool PCE_IsCD;

};
