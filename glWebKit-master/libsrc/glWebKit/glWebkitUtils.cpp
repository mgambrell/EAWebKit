#include "glWebKit/glWebKit.h"

#include "glWebkitUtils.h"

#include <GL/glew.h>

#include <EAWebKit/EAWebKit.h>
#include <EAWebKit/EAWebkitAllocator.h>
#include <EAWebKit/EAWebKitFileSystem.h>
#include <EAWebKit/EAWebKitClient.h>
#include <EAWebKit/EAWebKitView.h>
#include "EAWebkit/EAWebKitTextInterface.h"

#include <stdio.h>

#include <windows.h> // LoadLibraryA
#include <assert.h>
#include <array>

#include <vector>

#include <iostream>
#include <algorithm>

int add_ttf_font(EA::WebKit::EAWebKitLib* wk, const char* ttfFile) 
{
    EA::WebKit::ITextSystem* ts = wk->GetTextSystem();

    FILE* f = 0;
    fopen_s(&f, ttfFile, "rb");
    if (!f) return 1;
    fseek(f, 0L, SEEK_END);
    size_t fileSize = ftell(f);
    fseek(f, 0L, SEEK_SET);
    char* buffer = (char*)calloc(fileSize + 6, 1);
    size_t read_bytes = fread(buffer, 1, fileSize, f);
    if (read_bytes != fileSize) 
    {
        // error!
        free(buffer);
        return 0;
    }

    //Text system will take ownership of this memory
    int numFaces = ts->AddFace(buffer, fileSize);

    return numFaces;
}

unsigned int vPbo[2] = { 0, 0 };
unsigned char* buffer[2] = { 0, 0 };
int index = 0;
int nextIndex = 0;

void updateGLTexture(EA::WebKit::View* v, unsigned int id)
{
   if (!v)
   {
      return;
   }

   int w, h;
   EA::WebKit::ISurface* surface = v->GetDisplaySurface();
   surface->GetContentDimensions(&w, &h);
   int dataSize = w * h * 4;

   index = (index + 1) % 2;
   nextIndex = (index + 1) % 2;

   //lazily create and map the PBOs
   if(vPbo[0] == 0)
   {
      glGenBuffers(2, vPbo);

      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, vPbo[0]);
      glBufferStorage(GL_PIXEL_UNPACK_BUFFER, dataSize, 0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
      buffer[0] = (unsigned char*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, vPbo[1]);
      glBufferStorage(GL_PIXEL_UNPACK_BUFFER, dataSize, 0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
      buffer[1] = (unsigned char*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
   }

   EA::WebKit::ISurface::SurfaceDescriptor sd = {};
   surface->Lock(&sd);

   //copy the last frame's data to the texture.  This is a frame of lag, but should allow for fast transfers
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, vPbo[index]);
   glTextureSubImage2D(id, 0, 0, 0, w, h, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
   
   //copy the current data to the next buffer
   if(buffer)
   {
      //This image is actually flipped, but we're going to handle that in the shader instead of trying to actually flip
      //it on the CPU
      memcpy(buffer[nextIndex], sd.mData, w * h * 4);
   }
   
   surface->Unlock();   
}

bool evaluateJavaScript(EA::WebKit::View* v, const char* src, EA::WebKit::JavascriptValue* result)
{
   return v->EvaluateJavaScript(src, result);
}

void bindJavascriptObject(EA::WebKit::View* v, const char* name, EA::WebKit::IJSBoundObject* obj)
{
   v->BindJavaScriptObject(name, obj);
}
