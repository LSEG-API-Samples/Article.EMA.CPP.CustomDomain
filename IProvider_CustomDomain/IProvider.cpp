///*|-----------------------------------------------------------------------------
// *|            This source code is provided under the Apache 2.0 license      --
// *|  and is provided AS IS with no warranty or guarantee of fit for purpose.  --
// *|                See the project's LICENSE.md for details.                  --
// *|           Copyright Thomson Reuters 2016. All rights reserved.            --
///*|-----------------------------------------------------------------------------

#include "IProvider.h"
#include <fstream>
#include <iostream>

using namespace thomsonreuters::ema::access;
using namespace thomsonreuters::ema::rdm;
using namespace std;

UInt64 itemHandle = 0;

void AppClient::processLoginRequest( const ReqMsg& reqMsg, const OmmProviderEvent& event )
{
	event.getProvider().submit(RefreshMsg().domainType(MMT_LOGIN).name(reqMsg.getName()).nameType(USER_NAME).complete().
		attrib( ElementList().complete() ).solicited( true ).state( OmmState::OpenEnum, OmmState::OkEnum, OmmState::NoneEnum, "Login accepted" ) ,
		event.getHandle() );
}

void AppClient::processSymbolListRequest(const ReqMsg& reqMsg, const OmmProviderEvent& event)
{
	//Verify if the requested name is "FILE_LIST"
	if (reqMsg.getName() != "FILE_LIST")
	{
		event.getProvider().submit(StatusMsg().name(reqMsg.getName()).serviceName(reqMsg.getServiceName()).
			domainType(reqMsg.getDomainType()).
			state(OmmState::ClosedEnum, OmmState::SuspectEnum, OmmState::NotFoundEnum, "Only FIELD_LIST name is supported"),
			event.getHandle());
		return;
	}

	//List files in the "Files" folder.
	std::vector<FileInfo>* fileList = ListFile();

	Map map;
	// Iterate and print values of vector
	for (FileInfo n : *fileList) {
		//PARCL_SIZE
		map.addKeyAscii(n.filename.c_str(), MapEntry::AddEnum, FieldList().addRealFromDouble(1351, n.filesize).complete());
	}
	map.complete();

	event.getProvider().submit(RefreshMsg().domainType(MMT_SYMBOL_LIST).name(reqMsg.getName()).serviceName(reqMsg.getServiceName()).solicited(true).
		state(OmmState::OpenEnum, OmmState::OkEnum, OmmState::NoneEnum, "Refresh Completed").
		payload(map).complete(), event.getHandle());

	delete fileList;
}

void AppClient::processCustomRequest(const ReqMsg& reqMsg, const OmmProviderEvent& event)
{
	std::ifstream inputFile;
	std::string filepath = "Files\\";
	filepath.append(reqMsg.getName().c_str());
	inputFile.open(filepath, ios::in | ios::binary);
	if (inputFile.is_open())
	{
		//find length of opened file.
		inputFile.seekg(0, inputFile.end);
		int length = inputFile.tellg();
		inputFile.seekg(0, inputFile.beg);

		//define size of payload in each Refresh message
		int size = 65535;
		int partN = 0;
		char * buffer = new char[size];

		while (!inputFile.eof())
		{
			RefreshMsg refreshMsg;
			int currentIndex = inputFile.tellg();
			// read data as a block:
			inputFile.read(buffer, size);

			//Verify whether this message is the last part.
			if (!inputFile.eof())
			{
				//partial refresh
				event.getProvider().submit(refreshMsg.doNotCache(true).domainType(MMT_CUSTOM).name(reqMsg.getName()).serviceName(reqMsg.getServiceName()).solicited(true).
					state(OmmState::OpenEnum, OmmState::OkEnum, OmmState::NoneEnum, "Partial Refresh").
					complete(false).partNum(partN).
					payload(OmmOpaque().set(EmaBuffer(buffer, size)))
					, event.getHandle());
			}
			else
			{
				//complete refresh
				event.getProvider().submit(RefreshMsg().doNotCache(true).domainType(MMT_CUSTOM).name(reqMsg.getName()).serviceName(reqMsg.getServiceName()).solicited(true).
					state(OmmState::OpenEnum, OmmState::OkEnum, OmmState::NoneEnum, "Refresh Complete").complete(true).partNum(partN).
					payload(OmmOpaque().set(EmaBuffer(buffer, length- currentIndex)))
					, event.getHandle());
			}
			partN++;
		}

		delete[] buffer;
		inputFile.close();
	}
	else
	{
		processInvalidItemRequest(reqMsg, event);
		return;
	}
}

std::vector<FileInfo>* AppClient::ListFile()
{
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;
	
	std::regex e(".+\\..+\\..+\\..+");

	// Check that the input path plus 3 is not longer than MAX_PATH.
	// Three characters are for the "\*" plus NULL appended below.

	StringCchLength(".", MAX_PATH, &length_of_arg);

	if (length_of_arg > (MAX_PATH - 3))
	{
		cout << "\nDirectory path is too long.\n";
		return 0;
	}

	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.

	StringCchCopy(szDir, MAX_PATH, "Files");
	StringCchCat(szDir, MAX_PATH, "\\*.*");

	// Find the first file in the directory.

	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		cout << "FindFirstFile";
	}

	// List all the files in the directory with some info about them.
	std::vector<FileInfo>* fileList = new std::vector<FileInfo>();

	do
	{
		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
		{
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			
			if (std::regex_match(ffd.cFileName, e))
			{
				FileInfo f;
				f.filename.append(ffd.cFileName);
				f.filesize = filesize.QuadPart;
				fileList->push_back(f);
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
	{
		cout << "FindFirstFile";
	}

	FindClose(hFind);
	return fileList;
}

void AppClient::processDirectoryRequest(const ReqMsg& reqMsg, const OmmProviderEvent& event)
{
	event.getProvider().submit(RefreshMsg().domainType(MMT_DIRECTORY).filter(SERVICE_INFO_FILTER | SERVICE_STATE_FILTER).
		payload(Map().
			addKeyUInt(2, MapEntry::AddEnum, FilterList().
				add(SERVICE_INFO_ID, FilterEntry::SetEnum, ElementList().
					addAscii(ENAME_NAME, "INTERNAL_FEED").
					addArray(ENAME_CAPABILITIES, OmmArray().
						addUInt(MMT_SYMBOL_LIST).
						addUInt(MMT_CUSTOM).
						complete()).
					addArray(ENAME_DICTIONARYS_USED, OmmArray().
						addAscii("RWFFld").
						addAscii("RWFEnum").
						complete()).
					complete()).
				add(SERVICE_STATE_ID, FilterEntry::SetEnum, ElementList().
					addUInt(ENAME_SVC_STATE, SERVICE_UP).
					complete()).
				complete()).
			complete()).complete().solicited(true), event.getHandle());
}

void AppClient::processInvalidItemRequest(const ReqMsg& reqMsg, const OmmProviderEvent& event)
{
	event.getProvider().submit(StatusMsg().name(reqMsg.getName()).serviceName(reqMsg.getServiceName()).
		domainType(reqMsg.getDomainType()).
		state(OmmState::ClosedEnum, OmmState::SuspectEnum, OmmState::NotFoundEnum, "Item not found"),
		event.getHandle());
}

void AppClient::onReqMsg( const ReqMsg& reqMsg, const OmmProviderEvent& event )
{
	switch ( reqMsg.getDomainType() )
	{
	case MMT_LOGIN:
		processLoginRequest( reqMsg, event );
		break;
	case MMT_DIRECTORY:
		processDirectoryRequest( reqMsg, event );
		break;
	case MMT_SYMBOL_LIST:
		processSymbolListRequest(reqMsg, event);
		break;
	case MMT_CUSTOM:
		processCustomRequest(reqMsg, event);
		break;
	default:
		processInvalidItemRequest( reqMsg, event );
		break;
	}
}

int main( int argc, char* argv[] )
{
	try
	{
		AppClient appClient;

		OmmProvider provider( OmmIProviderConfig().adminControlDirectory( OmmIProviderConfig::UserControlEnum ).port("14002"), appClient );

		for (Int32 i = 0; i < 600; i++)
		{
			sleep(1000);
		}
	}
	catch ( const OmmException& excp )
	{
		cout << excp << endl;
	}
	
	return 0;
}
