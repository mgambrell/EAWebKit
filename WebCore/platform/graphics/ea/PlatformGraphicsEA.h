#ifndef PlatformGraphicsEA_h_
#define PlatformGraphicsEA_h_

#include <wtf/Platform.h>
#include <wtf/Noncopyable.h>
#include <wtf/TypeCasts.h>
#include <wtf/FastMalloc.h>


#if PLATFORM(EA)

#include "PlatformDisplay.h"

namespace WebCore {

  class PlatformDisplayEA final : public PlatformDisplay {
  public:
    PlatformDisplayEA();
    virtual ~PlatformDisplayEA();

    //Display* native() const { return m_display; }

  private:
    virtual Type type() const override { return PlatformDisplay::Type::EA; }

    #if USE(EGL)
    void initializeEGLDisplay() override;
    #endif

    //Display* m_display;
    //bool m_ownedDisplay;
  };

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_PLATFORM_DISPLAY(PlatformDisplayEA, EA)

#endif // PLATFORM(EA)

#endif // PlatformGraphicsEA
