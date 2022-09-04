#pragma once

#include <EAWebKit/EAWebKit>
#include <stdio.h>

class MyFileSystem : public EA::WebKit::FileSystem
{
public:
	virtual ~MyFileSystem();

	class MyFileObject
	{
	public:
		FILE* f = nullptr;
	};

	FileObject CreateFileObject() override;
	void DestroyFileObject(FileObject fileObject) override;

	bool OpenFile(FileObject fileObject, const EA::WebKit::utf8_t* path, int openFlags, int createDisposition = kCDONone) override;
	FileObject OpenTempFile(const EA::WebKit::utf8_t* prefix, EA::WebKit::utf8_t* pDestPath) override;
	void CloseFile(FileObject fileObject);

	int64_t ReadFile(FileObject fileObject, void* buffer, int64_t size) override;
	bool WriteFile(FileObject fileObject, const void* buffer, int64_t size) override;
	int64_t GetFileSize(FileObject fileObject) override;
	bool SetFileSize(FileObject fileObject, int64_t size) override;
	int64_t GetFilePosition(FileObject fileObject) override;
	bool SetFilePosition(FileObject fileObject, int64_t position) override;
	bool FlushFile(FileObject fileObject) override;
	bool FileExists(const EA::WebKit::utf8_t* path) override;
	bool DirectoryExists(const EA::WebKit::utf8_t* path) override;
	bool RemoveFile(const EA::WebKit::utf8_t* path) override;
	bool DeleteDirectory(const EA::WebKit::utf8_t* path) override;
	bool GetFileSize(const EA::WebKit::utf8_t* path, int64_t& size) override;
	bool GetFileModificationTime(const EA::WebKit::utf8_t* path, time_t& result) override;
	bool MakeDirectory(const EA::WebKit::utf8_t* path) override;
	bool GetDataDirectory(EA::WebKit::utf8_t* path, size_t pathBufferCapacity) override;
	bool GetTempDirectory(EA::WebKit::utf8_t* path, size_t pathBufferCapacity) override;
};

class MyAllocatorSystem : public EA::WebKit::Allocator
{
public:
	virtual ~MyAllocatorSystem();

	void* Malloc(size_t size, int flags, const char* pName) override;
	void* MallocAligned(size_t size, size_t alignment, size_t offset, int flags, const char* pName) override;
	void Free(void* p, size_t size) override;
	void* Realloc(void* p, size_t size, int flags) override;

	//BLAAAAAAAA for some reason this is needed while debugging on windows
	bool SupportsOSMemoryManagement() override
	{
		return true;
	}

	size_t SystemPageSize() override
	{
		return 4 * 1024; //4K
	}

	void* ReserveUncommitted(size_t bytes, bool writable, bool executable) override;
	void* ReserveAndCommit(size_t bytes, bool writable, bool executable) override;
	void ReleaseDecommitted(void* address, size_t bytes) override;
	void* ReserveAndCommitAligned(size_t bytes, size_t alignment, void*& reserveBase, size_t& reserveSize, bool writable, bool executable) override;
	void ReleaseDecommittedAligned(void* reserveBase, size_t reserveSize, size_t alignment) override;
	void Commit(void* address, size_t bytes, bool writable, bool executable) override;
	void Decommit(void* address, size_t bytes) override;
};
