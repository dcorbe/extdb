#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/Path.h>
#include <Poco/URI.h>
#include <Poco/Exception.h>
#include <string>

//typedef Poco::Tuple<std::string, std::string, int> VAC_INFO;
/*
bool sanitize_steamid

{
    bool bRejected=false; // has strName been rejected?
 
    // Step through each character in the string until we either hit
    // the end of the string, or we rejected a character
    for (unsigned int nIndex=0; nIndex < strName.length() && !bRejected; nIndex++)
    {
        // If the current character is an alpha character, that's fine
        if (isalpha(strName[nIndex]))
            continue;
 
        // If it's a space, that's fine too
        if (strName[nIndex]==' ')
            continue;
 
        // Otherwise we're rejecting this input
        bRejected = true;
    }
}
*/
std::string DB_RAW::callProtocol(AbstractExt *extension, std::string input_str)
{
	try
	{
		Poco::Data::Session db_session = extension->getDBSession_mutexlock();
		Poco::Data::Statement select(db_session);
		select << ("SELECT \"Number of Vac Bans\" FROM `VAC BANS` where SteamID="  + steam_id);
		select.execute();
		Poco::Data::RecordSet rs(select);
		
		/*
		if not null 
		
		
			Poco::URI uri(argv[1]);
			Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());

			// prepare path
			std::string path(uri.getPathAndQuery());
			if (path.empty()) path = "/";

			// send request
			Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1);
			session.sendRequest(req);

			// get response
			Poco::Net::HTTPResponse res;
			//http://www.appinf.com/docs/poco/Poco.Net.HTTPResponse.html#17176
			std::cout << res.getStatus() << " " << res.getReason() << std::endl;

			// print response
			istream &is = session.receiveResponse(res);

			boost::property_tree::ptree pt;
			boost::property_tree::read_json(is, pt);

			cout << pt.get<string >("players..SteamId", "FAILED") << endl;
			cout << pt.get<string >("players..VACBanned", "FAILED") << endl;
			cout << pt.get<string >("players..NumberOfVACBans", "FAILED") << endl;
			cout << pt.get<string >("players..DaysSinceLastBan", "FAILED") << endl;
			cout << pt.get<string >("players..EconomyBan", "FAILED") << endl;
		*/
	}
	catch (Poco::Exception &ex)
	{
		cerr << ex.displayText() << endl;
	}
}