///*|-----------------------------------------------------------------------------
// *|            This source code is provided under the Apache 2.0 license      --
// *|  and is provided AS IS with no warranty or guarantee of fit for purpose.  --
// *|                See the project's LICENSE.md for details.                  --
// *|           Copyright Refinitiv 2021. All rights reserved.            --
///*|-----------------------------------------------------------------------------

#include "Consumer.h"

using namespace refinitiv::ema::access;
using namespace refinitiv::ema::rdm;
using namespace std;

void AppClient::onRefreshMsg( const RefreshMsg& refreshMsg, const OmmConsumerEvent& evt)
{
	cout << endl << "DomainType: " << (refreshMsg.getDomainType()) << endl
		<< "Item Name: " << (refreshMsg.hasName() ? refreshMsg.getName() : EmaString("<not set>")) << endl
		<< "Service Name: " << (refreshMsg.hasServiceName() ? refreshMsg.getServiceName() : EmaString("<not set>")) << endl;
			
	cout << "Item State: " << refreshMsg.getState().toString() << endl;
	cout << "Completed: " << (refreshMsg.getComplete()) << endl;

	if (refreshMsg.getDomainType() == MMT_SYMBOL_LIST && DataType::MapEnum == refreshMsg.getPayload().getDataType())
		decode(refreshMsg.getPayload().getMap());
	else if (refreshMsg.getDomainType() == MMT_CUSTOM && DataType::OpaqueEnum == refreshMsg.getPayload().getDataType())
		decode(refreshMsg.getPayload().getOpaque(), refreshMsg.getComplete(), static_cast<std::ofstream*>(evt.getClosure()));

}

void AppClient::onUpdateMsg( const UpdateMsg& updateMsg, const OmmConsumerEvent& )
{
	//There shoud be update message received for this application.
	cout << endl << "Item Name: " << ( updateMsg.hasName() ? updateMsg.getName() : EmaString( "<not set>" ) ) << endl
		<< "Service Name: " << (updateMsg.hasServiceName() ? updateMsg.getServiceName() : EmaString( "<not set>" ) ) << endl;
}

void AppClient::onStatusMsg( const StatusMsg& statusMsg, const OmmConsumerEvent& )
{
	cout << endl << "Item Name: " << ( statusMsg.hasName() ? statusMsg.getName() : EmaString( "<not set>" ) ) << endl
		<< "Service Name: " << (statusMsg.hasServiceName() ? statusMsg.getServiceName() : EmaString( "<not set>" ) ) << endl;

	if ( statusMsg.hasState() )
		cout << endl << "Item State: " << statusMsg.getState().toString() << endl;
}

void AppClient::decode( const Map& map )
{
	bool firstEntry = true;

	while ( map.forth() )
	{
		const MapEntry& me = map.getEntry();

		if ( me.getKey().getDataType() == DataType::AsciiEnum )
		{
			if (firstEntry)
			{
				
				//log column header to console
				cout << endl << "File Name\t\t\tFile Size" << endl;
				//store first file name in the list to be requested.
				fileName.set(me.getKey().getAscii());
			}
			cout << me.getKey().getAscii();
			if (me.getLoadType() == DataType::FieldListEnum)
			{
				cout << "\t";
				const FieldList& fl = me.getFieldList();
				while (fl.forth())
				{
					const FieldEntry& fe = fl.getEntry();
					if (fe.getFieldId() == 1351)
					{
						if (fe.getCode() == Data::BlankEnum)
							cout << " blank";
						else if (fe.getLoadType() == DataType::RealEnum)
						{
							cout << fe.getReal().getAsDouble() << " Bytes";		
						}
						if (firstEntry)
						{
							firstEntry = false;
							filesize = fe.getReal().getAsDouble();
						}
					}
				}
			}
		}
		cout << endl;
	}
}


void AppClient::decode(const OmmOpaque& op, bool isComplete, std::ofstream* file)
{
	EmaBuffer buffer = op.getBuffer();
	file->write(buffer.c_buf(), buffer.length());
		if (isComplete)
	{
		int size = file->tellp();
		if (size == filesize)
			cout << "Successfully receive complete data, size: " << filesize << endl;
		else
			cout << "Fail to receive complete data, size: " << size << ","<< filesize<< endl;

		file->close();
	}
}


int main( int argc, char* argv[] )
{
	try {
		AppClient client;
		OmmConsumer consumer( OmmConsumerConfig().host( "192.168.27.53:14002" ).username( "user1" ) );
		UInt64 handle = consumer.registerClient( ReqMsg().domainType( MMT_SYMBOL_LIST ).interestAfterRefresh(false).serviceName( "INTERNAL_FEED" ).name( "FILE_LIST" ), client );
		
		//Waiting until the list is received
		while (client.fileName == "")
			sleep(1000);
		
		//Open file for writing data
		std::ofstream outFileStream;
		outFileStream.open(client.fileName.c_str(), ios::out | ios::binary);

		handle = consumer.registerClient(ReqMsg().domainType(MMT_CUSTOM).interestAfterRefresh(false).serviceName("INTERNAL_FEED").name(client.fileName), client, &(client.myFile));
		
		sleep( 60000 );				// API calls onRefreshMsg(), onUpdateMsg(), or onStatusMsg()
	}
	catch ( const OmmException& excp ) {
		cout << excp << endl;
	}
	return 0;
}
