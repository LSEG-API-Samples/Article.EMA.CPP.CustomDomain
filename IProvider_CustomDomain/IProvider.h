///*|-----------------------------------------------------------------------------
// *|            This source code is provided under the Apache 2.0 license      --
// *|  and is provided AS IS with no warranty or guarantee of fit for purpose.  --
// *|                See the project's LICENSE.md for details.                  --
// *|           Copyright Thomson Reuters 2016. All rights reserved.            --
///*|-----------------------------------------------------------------------------

#ifndef __ema_iprovider_h_
#define __ema_iprovider_h_

#include <iostream>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "Ema.h"
#include <regex>
#include <vector>
#include <string>
#include <strsafe.h>

#include <sys/stat.h>

#pragma comment(lib, "User32.lib")

#define MMT_CUSTOM 129

void sleep(int millisecs)
{
#if defined WIN32
	::Sleep((DWORD)(millisecs));
#else
	struct timespec sleeptime;
	sleeptime.tv_sec = millisecs / 1000;
	sleeptime.tv_nsec = (millisecs % 1000) * 1000000;
	nanosleep(&sleeptime, 0);
#endif
}

struct FileInfo
{
	std::string filename;
	int filesize;
};

class AppClient : public thomsonreuters::ema::access::OmmProviderClient
{
public:

	void processLoginRequest(const thomsonreuters::ema::access::ReqMsg&, const thomsonreuters::ema::access::OmmProviderEvent&);

	void processDirectoryRequest(const thomsonreuters::ema::access::ReqMsg&, const thomsonreuters::ema::access::OmmProviderEvent&);

	void processInvalidItemRequest(const thomsonreuters::ema::access::ReqMsg&, const thomsonreuters::ema::access::OmmProviderEvent&);

	void processSymbolListRequest(const thomsonreuters::ema::access::ReqMsg&, const thomsonreuters::ema::access::OmmProviderEvent&);
	
	void processCustomRequest(const thomsonreuters::ema::access::ReqMsg&, const thomsonreuters::ema::access::OmmProviderEvent&);

	std::vector<FileInfo>*  ListFile();

protected:

	void onReqMsg(const thomsonreuters::ema::access::ReqMsg&, const thomsonreuters::ema::access::OmmProviderEvent&);
};

#endif // __ema_iprovider_h_
