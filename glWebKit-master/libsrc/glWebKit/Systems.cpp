#include "Systems.h"

#include <stdio.h>
#include <stdlib.h>

[[noreturn]]
static void mywk_abort(const char* why)
{
  printf(why);
  abort();
}


MyFileSystem::~MyFileSystem(){ }

MyFileSystem::FileObject MyFileSystem::CreateFileObject() { return (uintptr_t)(new MyFileObject()); }
void MyFileSystem::DestroyFileObject(FileObject fileObject) { delete (MyFileObject*)fileObject; }

bool MyFileSystem::OpenFile(FileObject fileObject, const EA::WebKit::utf8_t* path, int openFlags, int createDisposition)
{
  MyFileObject* mf = (MyFileObject*)fileObject;
  mf->f = fopen(path,openFlags & kWrite ? "wb" : "rb");
  return !!mf->f;
}

MyFileSystem::FileObject MyFileSystem::OpenTempFile(const EA::WebKit::utf8_t* prefix, EA::WebKit::utf8_t* pDestPath)
{
  mywk_abort("OpenTempFile");
}

void MyFileSystem::CloseFile(FileObject fileObject)
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
int64_t MyFileSystem::ReadFile(FileObject fileObject, void* buffer, int64_t size)
{
  MyFileObject* mf = (MyFileObject*)fileObject;
  auto result = fread(buffer,1,size,mf->f);
  return result;
}

bool MyFileSystem::WriteFile(FileObject fileObject, const void* buffer, int64_t size)
{
  MyFileObject* mf = (MyFileObject*)fileObject;
  fwrite(buffer,1,size,mf->f);
  return true;
}

int64_t MyFileSystem::GetFileSize(FileObject fileObject)
{
  MyFileObject* mf = (MyFileObject*)fileObject;
  auto at = ftell(mf->f);
  fseek(mf->f,0,SEEK_END);
  long len = ftell(mf->f);
  fseek(mf->f,at,SEEK_SET);
  return len;
}

bool MyFileSystem::SetFileSize(FileObject fileObject, int64_t size)
{
  mywk_abort("OpenTempFile");
  return false;
}

int64_t MyFileSystem::GetFilePosition(FileObject fileObject)
{
  MyFileObject* mf = (MyFileObject*)fileObject;
  return ftell(mf->f);
}

bool MyFileSystem::SetFilePosition(FileObject fileObject, int64_t position)
{
  MyFileObject* mf = (MyFileObject*)fileObject;
  fseek(mf->f,position,SEEK_SET);
  return true;
}

bool MyFileSystem::FlushFile(FileObject fileObject)
{
  //nop
  return false;
}

// File system functionality
bool MyFileSystem::FileExists(const EA::WebKit::utf8_t* path)
{
  FILE* inf = fopen(path,"rb");
  if(inf)
  {
    fclose(inf);
    return true;
  }
  return false;
}

bool MyFileSystem::DirectoryExists(const EA::WebKit::utf8_t* path)
{
  mywk_abort("DirectoryExists");
}
bool MyFileSystem::RemoveFile(const EA::WebKit::utf8_t* path)
{
  mywk_abort("RemoveFile");
}
bool MyFileSystem::DeleteDirectory(const EA::WebKit::utf8_t* path)
{
  mywk_abort("DeleteDirectory");
}
bool MyFileSystem::GetFileSize(const EA::WebKit::utf8_t* path, int64_t& size)
{
  mywk_abort("GetFileSize");
}
bool MyFileSystem::GetFileModificationTime(const EA::WebKit::utf8_t* path, time_t& result)
{
  mywk_abort("GetFileModificationTime");
}
bool MyFileSystem::MakeDirectory(const EA::WebKit::utf8_t* path)
{
  mywk_abort("MakeDirectory");
}
bool MyFileSystem::GetDataDirectory(EA::WebKit::utf8_t* path, size_t pathBufferCapacity)
{
  mywk_abort("GetDataDirectory");
}

bool MyFileSystem::GetTempDirectory(EA::WebKit::utf8_t* path, size_t pathBufferCapacity)
{ 
  path[0] = 0; 
  return false;
}
