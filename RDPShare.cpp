#include "Server.h"
#include "Sharer.h"
#include "Utils.h"
#include "RDPFile.h"
#include <sstream>

#include <Windows.h>

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

static const char* usage =
"Usage:\tRDPShare [idle] [http <port>] [rdp <port>] [screen {0..n}]\n"
"Defaults are: http 8080 rdp 3390 screen 0\n"
"A WDS session is started automatically, unless the \"idle\" option has been given.\n\n"
"Use a web browser to connect to the HTTP port for additional options.\n"
"In addition, the following requests are recognized when when sent to the HTTP port:\n"
" start: start WDS session\n"
" stop: stop WDS session\n"
" quit: exit server process\n"
" set_Resolution: set screen resolution\n"
" get_ConnectionString: get WDS connection string\n"
" get_RASessionID: get Remote Assistance session ID\n"
" get_SharedRect: get shared rect\n";

int
CALLBACK WinMain(HINSTANCE, HINSTANCE, char* inArgs, int)
{
	std::string command = GetProgramName() + " " + inArgs;

	unsigned short http = 8080, rdp = 3390, screen = 0;
	bool help = false, idle = false;

	std::istringstream iss(inArgs);
	std::string token, error;
	while( iss >> token && error.empty() )
	{
		if (token == "http" && iss >> http)
			;
		else if (token == "rdp" && iss >> rdp)
			;
		else if (token == "screen" && iss >> screen)
			;
		else if (token == "idle")
			idle = true;
		else if (token == "/?" || token.find("help") != std::string::npos)
			help = true;
		else
			error = "unexpected input: \"" + token + "\"";
	}
	if(help)
	{
        ::MessageBoxA(NULL, usage, command.c_str(), MB_OK);
		return 0;
	}
	else if(!error.empty())
		error = "Error: " + error + "\n\n" + usage;
	int result = error.empty() ? 0 : -1;

	::OleInitialize(nullptr);
	if( result == 0 ) try
	{
		Sharer sharer;
		sharer.SetPort(rdp);
		struct : Server
		{
			enum { start, stop };
			std::string OnRequest( const std::string& s) override
			{
				std::string response = "ok\r\n";
				if (s == "quit")
					Terminate();
				else if (s == "start")
					AsyncMessage(start);
				else if (s == "stop")
					AsyncMessage(stop);
				else if (s == "get_ConnectionString")
					response = connectionString;
				else if (s == "get_SharedRect")
					response = sharedRect;
				else if (s == "get_RASessionID")
					response = raInfo.SessionID;
				else if (s.find("set_ScreenSize") == 0)
					response = OnSetScreenSize(s.substr(::strlen("set_ScreenSize")));
				else if (s.find("GET /") == 0)
					response = OnHttpGet( s.substr(::strlen("GET /")));
				else
					response = "error: unknown request\r\n";
				return response;
			}
			void OnAsyncMessage(int msg, void*) override
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
			int Run()
			{
				return Server::Run(addr, httpport);
			}
			void Start()
			{
				error.clear();
				RECT r;
				if (GetScreenRect(screen, &r))
				{
					try
					{
						pSharer->SetRect(r).Start();
						connectionString = pSharer->ConnectionString();
						std::ostringstream oss;
						oss << r.left << " " << r.top << " " << r.right << " " << r.bottom << "\n";
						sharedRect = oss.str();
						ParseConnectionString(connectionString, raInfo);
					}
					catch( HR_SucceedOrDie hr)
					{
						error = GetErrorText(hr);
					}
					catch(...)
					{
						error = "unknown error";
					}
				}
				else
				{
					std::ostringstream oss;
					oss << "screen " << screen << " not found";
					error = oss.str();
				}
			}
			void Stop()
			{
				sharedRect.clear();
				connectionString.clear();
				pSharer->Stop();
			}
			std::string OnSetScreenSize(const std::string s)
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
			std::string OnHttpGet( const std::string s)
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
					header = HttpHeader.Download("RDPShare_Connect_" + raInfo.IP + ".sh");
					content = ClientScript(raInfo.IP, raInfo.Port, httpport);
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
			std::string HtmlMainPage()
			{
				bool stopped = connectionString.empty();
				std::ostringstream oss;
				oss << "<html><head><title>RDPShare</title></head>"
					<< "<body>"
					<< "<p>RDPShare server on ports " << httpport << "/" << pSharer->Port() << " is "
					<< (stopped ? "stopped" : "running") << ".";
				if (!error.empty())
					oss << "</br>Could not start sharing session: " << error;
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
			std::string HtmlRedirect()
			{
				std::ostringstream oss;
				oss << "<html><head><meta http-equiv='Refresh' content='0; url=/'></head>"
					<< "<body><p>Reloading ...</p></body></html>";
				return oss.str();
			}
			int addr, httpport, screen;
			Sharer* pSharer;
			RemoteAssistanceInfo raInfo;
			std::string connectionString, sharedRect, error;
		} server;
		server.addr = INADDR_ANY;
		server.httpport = http;
		server.screen = screen;
		server.pSharer = &sharer;
		if(!idle)
	  		server.Start();
		result = server.Run();
	}
 	catch (HR_SucceedOrDie hr)
	{
		result = hr;
	}
	catch(WSA_SucceedOrDie)
	{
		result = ::WSAGetLastError();
	}
	catch(BOOL_SucceedOrDie)
	{
		result = ::GetLastError();
	}
	catch(const std::string& s)
	{
		result = -1;
		error = s;
	}
	if (result)
	{
		if(error.empty())
			error = GetErrorText(result);
		::MessageBoxA(NULL, error.c_str(), command.c_str(), MB_OK | MB_ICONERROR);
	}
	::OleUninitialize();
	return result;
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
	"OPTIONS=\"/f /cert-ignore -encryption +compression\"\n"
	"SESSIONID=`send_command get_RASessionID`\n"
	"exec $APP /v:" << ip << ":" << raport << " /shell-dir:$SESSIONID $OPTIONS\n"
	;
	return oss.str();
}
