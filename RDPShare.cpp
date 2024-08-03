#include "RDPShareServer.h"
#include "Sharer.h"
#include "Utils.h"
#include <sstream>
#include <Windows.h>

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

	HRESULT oleInitializeResult = ::OleInitialize(nullptr);
	if( result == 0 && SUCCEEDED(oleInitializeResult)) try
	{
		Sharer sharer;
		sharer.SetPort(rdp);
		RDPShareServer server(INADDR_ANY, http, screen, &sharer);
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
	if (SUCCEEDED(oleInitializeResult))
		::OleUninitialize();
	return result;
}
