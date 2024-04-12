#include "WebKit/ea/Api/EAWebKit/include/EAWebKit/DLLInterface.h"

namespace EA::WebKit
{
	class EAWebKitLib;
}
extern "C" EA::WebKit::EAWebKitLib* CreateEAWebkitInstance(void);

//can't figure out how to get the headers to work to do this.. oh well
#if defined(_MSC_VER) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
#define EXPORTME __declspec(dllexport)
#endif

extern "C" EXPORTME EA::WebKit::EAWebKitLib* WrapCreateEAWebkitInstance(void)
{
	return CreateEAWebkitInstance();
}