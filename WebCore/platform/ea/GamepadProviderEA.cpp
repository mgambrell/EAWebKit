/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "config.h"
#include "GamepadProvider.h"
#include <EAWebKit/EAWebKitClient.h>
#include "../PlatformGamepad.h"
#include "../GamepadProviderClient.h"

#if ENABLE(GAMEPAD)

#include <wtf/NeverDestroyed.h>

//MBG addition
//exporting wtf types outside the DLL is almost impossible, it causes the whole universe to get included
//so we're going to create a completely separate type and thunk it here where WTF is safe
namespace EA
{
	namespace WebKit
	{
		class EAWebKitClientGamepadThunk : public WebCore::PlatformGamepad
		{
		public:
			EAWebKitClientGamepadThunk(EAWebKitClientGamepad* eagp, unsigned index)
				: PlatformGamepad(index)
				, eagp(eagp)
			{
			}

			void init()
			{
				axes.resize(eagp->getNumAxes());
				buttons.resize(eagp->getNumButtons());
				eagp->setAxesStorage(axes.data());
				eagp->setButtonsStorage(buttons.data());
			}

			EAWebKitClientGamepad* eagp;

			Vector<double> axes;
			Vector<double> buttons;

			//TODO: we may need to set the updated value

			const Vector<double>& axisValues() const override
			{
				return axes;
			}

			const Vector<double>& buttonValues() const override
			{
				return buttons;
			}
		};
	}
}

namespace
{
	static Vector<WebCore::PlatformGamepad*> myPlatformGamepads;

	void UpdatePads()
	{
		EA::WebKit::EAWebKitClientGamepad* gamepads;
		int nGamepads;
		EA::WebKit::GetEAWebKitClient()->GetGamepads(&gamepads, &nGamepads);

		for(int i=0;i<nGamepads;i++)
		{
			auto& eagp = gamepads[i];
			if(myPlatformGamepads.size() <= i)
			{
				myPlatformGamepads.resize(myPlatformGamepads.size()+1);
				auto thunk = new EA::WebKit::EAWebKitClientGamepadThunk(&eagp,i);
				thunk->init();
				myPlatformGamepads[i] = thunk;
				eagp.thunk = thunk;
			}
		}
	}

}


namespace WebCore
{
	static GamepadProvider* sharedProvider = nullptr;
	static GamepadProviderClient* pClientCurr = nullptr;

	GamepadProvider& GamepadProvider::singleton()
	{
		if(!sharedProvider) {
			static NeverDestroyed<GamepadProvider> defaultProvider;
			sharedProvider = &defaultProvider.get();
		}

		return *sharedProvider;
	}

	void GamepadProvider::setSharedProvider(GamepadProvider& newProvider)
	{
		sharedProvider = &newProvider;
	}

	void GamepadProvider::startMonitoringGamepads(GamepadProviderClient* pClient)
	{
		pClientCurr = pClient;

		UpdatePads();

		for(int i=0;i<myPlatformGamepads.size();i++)
			pClient->platformGamepadConnected(*myPlatformGamepads[i]);
	}

	void GamepadProvider::stopMonitoringGamepads(GamepadProviderClient*)
	{
		pClientCurr = nullptr;
	}

	const Vector<PlatformGamepad*>& GamepadProvider::platformGamepads()
	{
		//needed?
		//pClientCurr->platformGamepadInputActivity();

		return myPlatformGamepads;
	}

} // namespace WebCore

#endif // ENABLE(GAMEPAD)
