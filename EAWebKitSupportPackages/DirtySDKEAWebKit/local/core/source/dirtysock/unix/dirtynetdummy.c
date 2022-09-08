#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "dirtyplatform.h"
#include "dirtynet.h"

int32_t SocketCreate(int32_t iThreadPrio)
{
	return 0;
}

int32_t SocketDestroy(uint32_t uFlags)
{
	return 9;
}

int32_t SocketClose(SocketT *pSocket)
{
	return 0;
}

void SocketRelease(SocketT *pSocket)
{
}

int32_t SocketBind(SocketT *pSocket, const struct sockaddr *name, int32_t namelen)
{
	return 0;
}

int32_t SocketInfo(SocketT *pSocket, int32_t iInfo, int32_t iData, void *pBuf, int32_t iLen)
{
	return 0;
}

int32_t SocketControl(SocketT *pSocket, int32_t option, int32_t data1, void *data2, void *data3)
{
	return 0;
}

int32_t SocketListen(SocketT *pSocket, int32_t backlog)
{
	return 0;
}

SocketT *SocketAccept(SocketT *pSocket, struct sockaddr *addr, int32_t *addrlen)
{
	return 0;
}

int32_t SocketConnect(SocketT *pSocket, struct sockaddr *name, int32_t namelen)
{
	return 0;
}

HostentT *SocketLookup(const char *text, int32_t timeout)
{
	return 0;
}

SocketT *SocketOpen(int32_t af, int32_t type, int32_t protocol)
{
	return 0;
}

int32_t SocketRecvfrom(SocketT *pSocket, char *buf, int32_t len, int32_t flags, struct sockaddr *from, int32_t *fromlen)
{
	return 0;
}

int32_t SocketSendto(SocketT *pSocket, const char *buf, int32_t len, int32_t flags, const struct sockaddr *to, int32_t tolen)
{
	return 0;
}

int32_t NetPrintfCode(const char *pFormat, ...)
{
	return 0;
}

uint32_t NetTick(void)
{
	return 0;
}
