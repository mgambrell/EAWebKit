#include "AudioSourceProvider.h"
#include "AudioBus.h"
#include "AudioFileReader.h"
#include "AudioDestination.h"
#include "AudioIOCallback.h"

#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>

#include <EAWebKit/EAWebKitClient.h>
#include <functional>

namespace WebCore
{
	PassRefPtr<AudioBus> createBusFromInMemoryAudioFile(const void* data, size_t dataSize, bool mixToMono, float sampleRate)
	{
		//NOTE: this did not seem to end up getting used :(
		//but it was good for learning

		auto *pClient = EA::WebKit::GetEAWebKitClient();

		EA::WebKit::EAWebKitClientAudioBufferInfo *info = pClient->AudioLoadBuffer(data, dataSize, mixToMono, sampleRate);

		//it would be cool to use audioBus->setChannelMemory but I don't know if that codepath is tested, and I don't know how to get a signal when the bus is freed
		RefPtr<AudioBus> audioBus = AudioBus::create(info->numChannels, info->numFrames, true);
		audioBus->setSampleRate(sampleRate);

		//therefore we must copy the data now :(
		//I don't understand what to do with mixToMono for now.
		//I'm ignoring it until I find out I need it

		//User was supposed to deinterleave this already
		float* pSrc = info->data;
		for(int i=0;i<info->numChannels;i++)
		{
			auto *ch = audioBus->channel(0);
			float* pDst = ch->mutableData();
			memcpy(pDst,pSrc,info->numFrames * sizeof(float));
			pSrc += info->numFrames;
		}

		delete info;

		return audioBus;
	}

	class AudioDestinationEA : public AudioDestination
	{
	private:
		RefPtr<AudioBus> audioBus;
		AudioIOCallback& cb;
		unsigned numberOfInputChannels;
		unsigned numberOfOutputChannels;
		float m_sampleRate;
		int lastFrameCount = -1;
		
		bool running = false;

	public:
		AudioDestinationEA(AudioIOCallback& cb, const String& inputDeviceId, unsigned numberOfInputChannels, unsigned numberOfOutputChannels, float sampleRate)
			: cb(cb)
			, numberOfInputChannels(numberOfInputChannels)
			, numberOfOutputChannels(numberOfOutputChannels)
			, m_sampleRate(sampleRate)
		{
		}

		void userCallback(float* buffer, int frameCount)
		{
			if(lastFrameCount != frameCount)
			{
				lastFrameCount = frameCount;
				audioBus = AudioBus::create(numberOfOutputChannels, frameCount, false);
			}
			for(int i=0;i<numberOfOutputChannels;i++)
				audioBus->setChannelMemory(i, buffer+frameCount*i, frameCount); //bytes or frames?
			cb.render(nullptr, audioBus.get(), frameCount);
		}

		void start() override
		{
			auto fun = [this](float* buffer, int frameCount) { userCallback(buffer, frameCount); };
			EA::WebKit::GetEAWebKitClient()->AudioStart(fun, numberOfInputChannels, numberOfOutputChannels, m_sampleRate);
			running = true;
		}

		void stop() override
		{
			EA::WebKit::GetEAWebKitClient()->AudioStop();
			running = false;
		}

		bool isPlaying() override
		{
			return running;
		}

		float sampleRate() const override
		{
			return m_sampleRate;
		}

	};

	std::unique_ptr<AudioDestination> AudioDestination::create(AudioIOCallback& cb, const String& inputDeviceId, unsigned numberOfInputChannels, unsigned numberOfOutputChannels, float sampleRate)
	{
		return std::make_unique<AudioDestinationEA>(cb, inputDeviceId, numberOfInputChannels, numberOfOutputChannels, sampleRate);
	}

	unsigned long WebCore::AudioDestination::maxChannelCount()
	{
		return 2;
	}

	float WebCore::AudioDestination::hardwareSampleRate()
	{
		return 48000;
	}

	PassRefPtr<AudioBus> AudioBus::loadPlatformResource(const char *name, float what)
	{
		//does this get used?
		return nullptr;
	}

} //namespace WebCore
