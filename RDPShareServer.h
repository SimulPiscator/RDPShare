#include "Server.h"
#include "RDPFile.h"

class Sharer;

class RDPShareServer: Server
{
public:
	RDPShareServer(int addr, int httpport, int screen, Sharer* pSharer);
	int Run();
	void Start();
	void Stop();

protected:
	std::string OnRequest(const std::string& s) override;
	enum { start, stop };
	void OnAsyncMessage(int msg, void*) override;

private:
	std::string OnSetScreenSize(const std::string s);
	std::string OnHttpGet(const std::string s);
	std::string HtmlMainPage();
	std::string HtmlRedirect();

	int mAddress, mHttpPort, mScreen;
	Sharer* mpSharer;
	RemoteAssistanceInfo mRaInfo;
	std::string mConnectionString, mSharedRect, mError;
};
