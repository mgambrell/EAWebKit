#include "PlatformGraphicsEA.h"

#include <EGL/egl.h>

#if PLATFORM(EA)

namespace WebCore {

PlatformDisplayEA::PlatformDisplayEA()
{
	int dummy=0;
}

PlatformDisplayEA::~PlatformDisplayEA()
{
	int dummy=0;
}

void PlatformDisplayEA::initializeEGLDisplay()
{
	//MBG TODO - well, in theory, we should be passing in a display handle (see PlatformDisplayX11)
	//but that's not done for now. not likely to matter anyway, we're just faking the displays here
	EGLNativeDisplayType displayHandle = {0};
	m_eglDisplay = eglGetDisplay(displayHandle);
	PlatformDisplay::initializeEGLDisplay();
}

}


#endif

