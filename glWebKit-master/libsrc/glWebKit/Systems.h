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
