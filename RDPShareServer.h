/*
 * RDPShare: Windows Desktop Sharing remote control interface
 *
 * Copyright 2016 Simul Piscator
 *
 * RDPShare is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RDPShare is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RDPShare.  If not, see <http://www.gnu.org/licenses/>.
 */
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
