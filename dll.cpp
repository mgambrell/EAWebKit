namespace EA::WebKit
{
	class EAWebKitLib;
}
extern "C" EA::WebKit::EAWebKitLib* CreateEAWebkitInstance(void);

extern "C" EA::WebKit::EAWebKitLib* WrapCreateEAWebkitInstance(void)
{
	return CreateEAWebkitInstance();
}