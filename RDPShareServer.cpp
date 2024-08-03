#include "RDPShareServer.h"
#include "Sharer.h"
#include "Utils.h"
#include <sstream>

static const struct
{
	std::string OK, NotFound;
	static std::string Download(const std::string& filename)
	{
		std::string header = "HTTP/1.1 200 OK\r\n"
			"Content-Type: application/octet-stream\r\n"
			"Content-Disposition: attachment; filename=\"" + filename + "\"\r\n";
		return header;
	}
} HttpHeader =
{
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: text/html\r\n",

	"HTTP/1.1 404 Not Found\r\n"
	"Content-Type: text/html\r\n",
};

static std::string HtmlLink(const std::string& text, const std::string& target, bool enabled);
static std::string ClientScript(const std::string& ip, const std::string& raport, int httpport);

std::string RDPShareServer::OnRequest(const std::string& s)
{
	std::string response = "ok\r\n";
	if (s == "quit")
		Terminate();
	else if (s == "start")
		AsyncMessage(start);
	else if (s == "stop")
		AsyncMessage(stop);
	else if (s == "get_ConnectionString")
		response = mConnectionString;
	else if (s == "get_SharedRect")
		response = mSharedRect;
	else if (s == "get_RASessionID")
		response = mRaInfo.SessionID;
	else if (s.find("set_ScreenSize") == 0)
		response = OnSetScreenSize(s.substr(::strlen("set_ScreenSize")));
	else if (s.find("GET /") == 0)
		response = OnHttpGet( s.substr(::strlen("GET /")));
	else
		response = "error: unknown request\r\n";
	return response;
}

void RDPShareServer::OnAsyncMessage(int msg, void*)
{
	switch( msg )	
	{
	case start:
		Start();
		break;
	case stop:
		Stop();
		break;
	}
}

RDPShareServer::RDPShareServer(int addr, int httpport, int screen, Sharer* pSharer)
: mAddress(addr), mHttpPort(httpport), mScreen(screen), mpSharer(pSharer)
{
}

int RDPShareServer::Run()
{
	return Server::Run(mAddress, mHttpPort);
}

void RDPShareServer::Start()
{
	mError.clear();
	RECT r;
	if (GetScreenRect(mScreen, &r))
	{
		try
		{
			mpSharer->SetRect(r).Start();
			mConnectionString = mpSharer->ConnectionString();
			std::ostringstream oss;
			oss << r.left << " " << r.top << " " << r.right << " " << r.bottom << "\n";
			mSharedRect = oss.str();
			ParseConnectionString(mConnectionString, mRaInfo);
		}
		catch( HR_SucceedOrDie hr)
		{
			mError = GetErrorText(hr);
		}
		catch(...)
		{
			mError = "unknown error";
		}
	}
	else
	{
		std::ostringstream oss;
		oss << "screen " << mScreen << " not found";
		mError = oss.str();
	}
}

void RDPShareServer::Stop()
{
	mSharedRect.clear();
	mConnectionString.clear();
	mpSharer->Stop();
}

std::string RDPShareServer::OnSetScreenSize(const std::string s)
{
	std::string screen, size;
	std::istringstream iss(s);
	iss >> screen >> size;
	if (size.empty())
	{
		size = screen;
		screen = "1";
	}
	int width = -1, height = -1, screenIdx = ::atoi(screen.c_str());
	size_t pos = size.find('x');
	if (pos < size.length())
	{
		width = ::atoi(size.c_str());
		height = ::atoi(size.c_str() + pos + 1);
	}
	if (width < 0 || height < 0)
		return "error: bad format: " + s + "\n";
	if (screenIdx == 0)
		return "error: cannot set desktop size, specify a screen > 0\n";
	RECT r;
	if (!GetScreenRect(screenIdx, &r))
		return "error: unknown screen " + screen + " \n";
	int result = SetScreenSize(screenIdx, width, height);
	std::string message = "error: could not set screen size";
	switch (result)
	{
	case DISP_CHANGE_SUCCESSFUL:
		message = "ok";
		break;
#define _(x)	case x: message += " (" #x ")"; break;
		_(DISP_CHANGE_BADDUALVIEW)
		_(DISP_CHANGE_BADFLAGS)
		_(DISP_CHANGE_BADMODE)
		_(DISP_CHANGE_BADPARAM)
		_(DISP_CHANGE_FAILED)
		_(DISP_CHANGE_RESTART)
		_(DISP_CHANGE_NOTUPDATED)
#undef _
	}
	return message + "\n";
}

std::string RDPShareServer::OnHttpGet( const std::string s)
{
	std::string header, content;
	if (s.find("?Start")==0)
	{
		AsyncMessage(start);
		header = HttpHeader.OK;
		content = HtmlRedirect();
	}
	else if (s.find("?Stop") == 0)
	{
		AsyncMessage(stop);
		header = HttpHeader.OK;
		content = HtmlRedirect();
	}
	else if (s.find("?ClientScript") == 0)
	{
		header = HttpHeader.Download("RDPShare_Connect_" + mRaInfo.IP + ".sh");
		content = ClientScript(mRaInfo.IP, mRaInfo.Port, mHttpPort);
	}				
	else
	{
		header = HttpHeader.OK;
		content = HtmlMainPage();
	}
	if (content.empty())
		header = HttpHeader.NotFound;
	std::ostringstream oss;
	oss << "Content-Length: " << content.length() << "\r\n\r\n";
	return header + oss.str() + content;
}

std::string RDPShareServer::HtmlMainPage()
{
	bool stopped = mConnectionString.empty();
	std::ostringstream oss;
	oss << "<html><head><title>RDPShare</title></head>"
		<< "<body>"
		<< "<p>RDPShare server on ports " << mHttpPort << "/" << mpSharer->Port() << " is "
		<< (stopped ? "stopped" : "running") << ".";
	if (!mError.empty())
		oss << "</br>Could not start sharing session: " << mError;
	oss << "</p>\n"
		<< HtmlLink(stopped ? "Start" : "Restart", "/?Start", true) << "</br>"
		<< HtmlLink("Stop", "/?Stop", !stopped) << "</br>"
		<< "<p>Download "
		<< HtmlLink("client wrapper shell script", "/?ClientScript", true) << "."
		<< "</br>The script depends on <tt>xfreerdp</tt> and <tt>netcat</tt>, and "
		<< "must be made executable using <tt>chmod 775</tt> before it can be used.</p>"
		<< "</body></html>";
	return oss.str();
}

std::string RDPShareServer::HtmlRedirect()
{
	std::ostringstream oss;
	oss << "<html><head><meta http-equiv='Refresh' content='0; url=/'></head>"
		<< "<body><p>Reloading ...</p></body></html>";
	return oss.str();
}

static std::string HtmlLink(const std::string& text, const std::string& target, bool enabled)
{
	std::string code = "<a ";
	if (enabled)
		code += "href='" + target + "'";
	else
		code += "disabled='true'";
	code += ">" + text + "</a>";
	return code;
}

static std::string ClientScript(const std::string& ip, const std::string& raport, int httpport)
{
	std::ostringstream oss;
	oss <<
	"#!/bin/sh\n"
	"send_command()\n"
	"{\n"
	"  echo $1 | nc " << ip << " " << httpport << "\n"
	"}\n"
	"send_command stop > /dev/null\n"
	"send_command start > /dev/null\n"
	"APP=xfreerdp\n"
	"OPTIONS=\"/f /cert:ignore /sec:rdp -encryption +compression /u:null@null /p:null\"\n"
	"SESSIONID=`send_command get_RASessionID`\n"
	"exec $APP /v:" << ip << ":" << raport << " /shell-dir:$SESSIONID $OPTIONS\n"
	;
	return oss.str();
}
