#include "glWebKit/glWebKit.h"

#include "glWebkitUtils.h"
#include "glWebkitRenderer.h"
#include "glWebkitThreading.h"
#include "glWebkitClient.h"

#include <EAWebKit\EAWebKit>

#include <windows.h>

#include <list>
#include <iostream>

EA::WebKit::EAWebKitLib* wk = nullptr;

// Callbacks
double timerCallback() 
{ 
    LARGE_INTEGER frequency;
    ::QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER start;
    ::QueryPerformanceCounter(&start);

    return static_cast<double>(start.QuadPart) / (double)frequency.QuadPart;
}

double monotonicTimerCallback() 
{
    return timerCallback();
};

bool cryptographicallyRandomValueCallback(unsigned char *buffer, size_t length)
{
  //not important (LOL!)
  return true;
}

void* stackBaseCallback() 
{
   //taken from: https://github.com/adobe/webkit/blob/master/Source/WTF/wtf/StackBounds.cpp#L228
   PNT_TIB64 pTib = reinterpret_cast<PNT_TIB64>(NtCurrentTeb());
   return reinterpret_cast<void*>(pTib->StackBase);
}


void* stackLimitCallback() 
{
  //taken from: https://github.com/adobe/webkit/blob/master/Source/WTF/wtf/StackBounds.cpp#L228
  PNT_TIB64 pTib = reinterpret_cast<PNT_TIB64>(NtCurrentTeb());
  return reinterpret_cast<void*>(pTib->StackLimit);
}



void getCookiesCallback(const char16_t* pUrl, EA::WebKit::EASTLFixedString16Wrapper& result, uint32_t flags)
{
   std::cout << __FUNCTION__ << std::endl;
}

bool setCookieCallback(const EA::WebKit::CookieEx& cookie)
{
   std::cout << __FUNCTION__ << std::endl;
   return false;  
}

struct EA::WebKit::AppCallbacks callbacks = {
   timerCallback,
   monotonicTimerCallback,
   stackBaseCallback,
   stackLimitCallback,
   cryptographicallyRandomValueCallback,
   getCookiesCallback,
   setCookieCallback
};

// init the systems: using DefaultAllocator, DefaultFileSystem, no text/font support, DefaultThreadSystem
struct EA::WebKit::AppSystems systems = { nullptr };

[[noreturn]]
void mywk_abort(const char* why)
{
  printf(why);
  abort();
}

class MyFileSystem : public EA::WebKit::FileSystem
{
public:
  virtual ~MyFileSystem(){ }

  class MyFileObject
  {
  public:
    FILE* f = nullptr;
  };

  FileObject CreateFileObject() override { return (uintptr_t)(new MyFileObject()); }
  void DestroyFileObject(FileObject fileObject) override { delete (MyFileObject*)fileObject; }

  bool OpenFile(FileObject fileObject, const EA::WebKit::utf8_t* path, int openFlags, int createDisposition =  kCDONone) override
  {
    MyFileObject* mf = (MyFileObject*)fileObject;
    mf->f = fopen(path,openFlags & kWrite ? "wb" : "rb");
    return !!mf->f;
  }

  FileObject OpenTempFile(const EA::WebKit::utf8_t* prefix, EA::WebKit::utf8_t* pDestPath) override
  {
    mywk_abort("OpenTempFile");
  }
  
  void CloseFile(FileObject fileObject)
  {
    MyFileObject* mf = (MyFileObject*)fileObject;
    if(mf->f)
    {
      fclose(mf->f);
      mf->f = nullptr;
    }
  }

  //MBG NOTE - FileSystemDefault's implementation made no sense. it seems this should just work in the sensible way and return what was read.
  // Returns (int64_t)(ReadStatus::kReadError) in case of an error that is not recoverable
  // if Returns > 0, the total number of bytes read.
  // if Returns ReadStatus::kReadComplete, the file read is complete and no more data to read.
  int64_t ReadFile(FileObject fileObject, void* buffer, int64_t size) override
  {
    MyFileObject* mf = (MyFileObject*)fileObject;
    auto result = fread(buffer,1,size,mf->f);
    return result;
  }

  virtual bool WriteFile(FileObject fileObject, const void* buffer, int64_t size) override
  {
    MyFileObject* mf = (MyFileObject*)fileObject;
    fwrite(buffer,1,size,mf->f);
    return true;
  }

  int64_t GetFileSize(FileObject fileObject) override
  {
    MyFileObject* mf = (MyFileObject*)fileObject;
    auto at = ftell(mf->f);
    fseek(mf->f,0,SEEK_END);
    long len = ftell(mf->f);
    fseek(mf->f,at,SEEK_SET);
    return len;
  }

  bool SetFileSize(FileObject fileObject, int64_t size) override
  {
    mywk_abort("OpenTempFile");
    return false;
  }
  
  int64_t GetFilePosition(FileObject fileObject) override
  {
    MyFileObject* mf = (MyFileObject*)fileObject;
    return ftell(mf->f);
  }

  bool SetFilePosition(FileObject fileObject, int64_t position) override
  {
    MyFileObject* mf = (MyFileObject*)fileObject;
    fseek(mf->f,position,SEEK_SET);
    return true;
  }

  bool FlushFile(FileObject fileObject) override
  {
    //nop
    return false;
  }

  // File system functionality
  bool FileExists(const EA::WebKit::utf8_t* path) override
  {
    FILE* inf = fopen(path,"rb");
    if(inf)
    {
      fclose(inf);
      return true;
    }
    return false;
  }
  
  bool DirectoryExists(const EA::WebKit::utf8_t* path) override
  {
    mywk_abort("DirectoryExists");
  }
  bool RemoveFile(const EA::WebKit::utf8_t* path) override
  {
    mywk_abort("RemoveFile");
  }
  bool DeleteDirectory(const EA::WebKit::utf8_t* path) override
  {
    mywk_abort("DeleteDirectory");
  }
  bool GetFileSize(const EA::WebKit::utf8_t* path, int64_t& size) override
  {
    mywk_abort("GetFileSize");
  }
  bool GetFileModificationTime(const EA::WebKit::utf8_t* path, time_t& result) override
  {
    mywk_abort("GetFileModificationTime");
  }
  bool MakeDirectory(const EA::WebKit::utf8_t* path) override
  {
    mywk_abort("MakeDirectory");
  }
  bool GetDataDirectory(EA::WebKit::utf8_t* path, size_t pathBufferCapacity) override
  {
    mywk_abort("GetDataDirectory");
  }

  virtual bool GetTempDirectory(EA::WebKit::utf8_t* path, size_t pathBufferCapacity) override
  { 
    path[0] = 0; 
    return false;
  }

} s_MyFileSystem;

bool initWebkit()
{
   systems.mThreadSystem = new StdThreadSystem;
   systems.mEAWebkitClient = new GLWebkitClient();
   systems.mFileSystem = &s_MyFileSystem;

   typedef EA::WebKit::EAWebKitLib* (*PF_CreateEAWebkitInstance)(void);
   PF_CreateEAWebkitInstance create_Webkit_instance = nullptr;

#ifdef _DEBUG
   HMODULE wdll = LoadLibraryA("EAWebkitd.dll");
#else
   HMODULE wdll = LoadLibraryA("EAWebkit.dll");
#endif // _DEBUG

   if(!wdll)
     abort();
   
   create_Webkit_instance = reinterpret_cast<PF_CreateEAWebkitInstance>(GetProcAddress(wdll, "CreateEAWebkitInstance"));
   
   if(!create_Webkit_instance)
     abort();

   wk = create_Webkit_instance();

   //check that dll is same version as our headers
   const char* verStr = wk->GetVersion();
   if(strcmp(verStr, EAWEBKIT_VERSION_S) != 0)
   {
      std::cout << "Error!  Mismatched versions of EA Webkit" << std::endl;
      return false;
   }

   //initialize the system
   wk->Init(&callbacks, &systems);

   EA::WebKit::Parameters& params = wk->GetParameters();
   params.mEAWebkitLogLevel = 4;
   params.mHttpManagerLogLevel = 4;
   params.mRemoteWebInspectorPort = 1234;
   params.mReportJSExceptionCallstacks = true;
   params.mVerifySSLCert = true;
   params.mJavaScriptStackSize = 2 * 1024 * 1024; //MBG MODIFIED

   wk->SetParameters(params);

   //should be pulling these from the OS by their family type
   //times new roman is the default fallback if a font isn't found, so we need 
   //to at least load this (should probably be built in)
   int ret = add_ttf_font(wk, "times.ttf");
   if (ret == 0)
   {
      std::cout << "Error adding times.ttf font. " << std::endl;
   }

   return true;
}

EA::WebKit::View* createView(int x, int y)
{
   EA::WebKit::View* v = 0;

   v = wk->CreateView();
   EA::WebKit::ViewParameters vp;
   vp.mHardwareRenderer = nullptr; // use default renderer
   vp.mDisplaySurface = nullptr; // use default surface
   vp.mWidth = x;
   vp.mHeight = y;
   vp.mBackgroundColor = 0; //clear  0xffffffff; //white
   vp.mTileSize = 256;
   vp.mUseTiledBackingStore = false;
   vp.mpUserData = v;
   v->InitView(vp);
   v->SetSize(EA::WebKit::IntSize(vp.mWidth, vp.mHeight));

   return v;
}

bool shutdownWebKit()
{
   wk->Shutdown();

   return true;
}

void updateWebkit()
{
   if (!wk) 
       return;

    wk->Tick();
    
}

void setViewUrl(EA::WebKit::View* v, const char* url)
{
   v->SetURI(url);
}

void updateView(EA::WebKit::View* v)
{
    v->Paint();
}

void resize(EA::WebKit::View* v, int width, int height)
{
    if (!v) 
       return;

    v->SetSize(EA::WebKit::IntSize(width, height));
}

void mousemove(EA::WebKit::View* v, int x, int y)
{
    if (!v) 
       return;

    EA::WebKit::MouseMoveEvent e = {};
    e.mX = x;
    e.mY = y;
    v->OnMouseMoveEvent(e);
}

void mousebutton(EA::WebKit::View* v, int x, int y, int btn, bool depressed)
{
    if (!v) 
       return;

    EA::WebKit::MouseButtonEvent e = {};
    e.mId = btn;
    e.mX = x;
    e.mY = y;
    e.mbDepressed = depressed;
    v->OnMouseButtonEvent(e);
}

void mousewheel(EA::WebKit::View* v, int x, int y, int keys, int delta)
{
    if (!v) 
       return;

    EA::WebKit::MouseWheelEvent e = {};
    e.mX = x;
    e.mY = y;
    e.mZDelta = delta;

    UINT scrollLines = 1;
    SystemParametersInfoA(SPI_GETWHEELSCROLLLINES, 0, &scrollLines, 0);
    e.mNumLines = ((delta * (int32_t)scrollLines) / (int32_t)WHEEL_DELTA);
    v->OnMouseWheelEvent(e);
}

void keyboard(EA::WebKit::View* v, int id, bool ischar, bool depressed)
{
    if (!v) 
       return;

    EA::WebKit::KeyboardEvent e = {};
    e.mId = id;
    e.mbChar = ischar;
    e.mbDepressed = depressed;
    v->OnKeyboardEvent(e);
}

void reload(EA::WebKit::View* v)
{
    if (!v)
       return;

    v->Refresh();
}

void destroyView(EA::WebKit::View* v)
{
   if(!wk || !v)
      return;

   wk->DestroyView(v);
}
