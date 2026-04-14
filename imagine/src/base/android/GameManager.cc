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

#include <imagine/base/GameManager.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/jni.hh>
#include <imagine/logger/SystemLogger.hh>

namespace IG
{

[[maybe_unused]] static SystemLogger log{"GameManager"};

static JNI::InstMethod<void(jboolean, jint)> jSetGameState{};

GameManager ApplicationContext::gameManager()
{
	if(androidSDK() < 33)
	{
		return {};
	}
	if(!jSetGameState)
	{
		auto env = mainThreadJniEnv();
		auto baseActivity = baseActivityObject();
		jSetGameState = {env, baseActivity, "setGameState", "(ZI)V"};
	}
	return {*this};
}

void GameManager::setGameState(GameState state)
{
	if(!jSetGameState)
		return;
	log.info("set game state mode:{} isLoading:{}", enumName(state.mode), state.isLoading);
	jSetGameState(ctx.mainThreadJniEnv(), ctx.baseActivityObject(), state.isLoading, to_underlying(state.mode));
}

GameManager::operator bool() const { return bool(jSetGameState); }

}
