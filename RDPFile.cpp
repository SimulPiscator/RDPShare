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
#include "RDPFile.h"
#include "Utils.h"
#include <vector>
#include <sstream>
#include <ctime>

static void ParseConnectionString(
	const std::string& s, 
	std::string& RASpecificParams, 
	std::string& RASessionID, 
	std::vector<std::pair<std::string,std::string>>& Addresses
)
{
	size_t begin = s.find("KH=\""), end = begin;
	if (begin != s.npos)
	{
		begin = s.find('"', begin) + 1;
		end = s.find('"', begin);
		RASpecificParams = s.substr(begin, end - begin);
	}
	begin = s.find("ID=\""), end = begin;
	if (begin != s.npos)
	{
		begin = s.find('"', begin) + 1;
		end = s.find('"', begin);
		RASessionID = s.substr(begin, end - begin);
	}
	begin = 0, end = 0;
	while ((begin = s.find("<L P=\"", end)) != s.npos)
	{
		begin = s.find('"', begin) + 1;
		end = s.find('"', begin);
		std::string port = s.substr(begin, end - begin);
		begin = s.find("N=\"", end);
		if (begin != s.npos)
		{
			begin = s.find('"', begin) + 1;
			end = s.find('"', begin);
			Addresses.push_back( std::make_pair(s.substr(begin, end - begin),port) );
		}
	}
	if (RASpecificParams.empty() || RASessionID.empty() || Addresses.empty())
		throw std::string("Failed to parse connection string");
}

bool
ParseConnectionString(const std::string& inString, RemoteAssistanceInfo& outInfo)
{
	std::string RASpecificParams;
	std::vector<std::pair<std::string, std::string>> Addresses;
	ParseConnectionString(inString, RASpecificParams, outInfo.SessionID, Addresses);
	outInfo.IP = Addresses.back().first;
	outInfo.Port = Addresses.back().second;
	return true;
}

#if 0
std::string
CreateRdpFile(const std::string& s, int width, int height, bool freerdpworkaround)
{
	std::string RASessionID, RASpecificParams;
	std::vector<std::pair<std::string,std::string>> Addresses;
	ParseConnectionString(s, RASpecificParams, RASessionID, Addresses);

	std::ostringstream rdpfile;
	if (freerdpworkaround)
		rdpfile
		// including port into full address triggers a bug in FreeRDP 1.1, so always use server port field for port
		<< "full address:s:" << Addresses.back().first << "\n"
		<< "server port:i:" << Addresses.back().second << "\n";
	else // ms clients seem to ignore server port field if full address is given
		rdpfile
		<< "full address:s:" << Addresses.back().first << ":" << Addresses.back().second << "\n";
	rdpfile
		<< "alternate shell:s:\n"
		<< "shell working directory:s:" << RASessionID << "\n"
		<< "authentication level:i:0\n"
		<< "audiomode:i:2\n"
		<< "prompt for credentials on client:i:0\n";
	if (freerdpworkaround)
		rdpfile
		<< "smart sizing:i:0\n"; // 1 doesn't work properly with FreeRDP 1.1
	else
		rdpfile
		<< "smart sizing:i:1\n";
	rdpfile
		<< "screen mode id:i:1\n" // window
		<< "username:s:rdpshare\n" // we don't care
		<< "desktopwidth:i:" << width << "\n"
		<< "desktopheight:i:" << height << "\n";
	;
	return rdpfile.str();
}

std::string RdpFileToUri(const std::string& s)
{
	std::string uri = "rdp://";
	std::istringstream iss(s);
	std::string line;
	while(getline(iss, line))
	{
		size_t pos1 = line.find(':'), pos2 = line.find(':', pos1 + 1);
		uri += UrlEscape_(line.substr(0,pos1));
		uri += "=";
		uri += line.substr(pos1 + 1, pos2 - pos1);
		uri += UrlEscape_(line.substr(pos2 + 1),".:=");
		uri += "&";
	}
	uri.pop_back();
	return uri;
}

std::string RdpFileToCmdline(const std::string& s)
{
	std::string cmd;
	std::istringstream iss(s);
	std::string line;
	while (getline(iss, line))
	{
		size_t pos1 = line.find(':'), pos2 = line.find(':', pos1 + 1);
		std::string var = line.substr(0, pos1), val = line.substr(pos2 + 1);
		if (var == "full address")
			cmd += "\"/v:" + val + "\" ";
		else if( var == "shell working directory")
			cmd += "\"/shell-dir:" + val + "\" ";
	}
	return cmd;
}

std::string CreateTicket(const std::string& s)
{
	std::string RASessionID, RASpecificParams, AddressList;
	std::vector<std::pair<std::string, std::string>> Addresses;
	ParseConnectionString(s, RASpecificParams, RASessionID, Addresses);
	for (const auto& address : Addresses)
		AddressList += ";" + address.first + ":" + address.second;

	std::ostringstream ticket;
	ticket << "<?xml version=\"1.0\"?>\r\n"
		<< "<UPLOADINFO TYPE=\"Escalated\"> "
		<< "<UPLOADDATA USERNAME=\"rdpshare\" "
		<< "RCTICKET=\"65538,1," 
		<< AddressList.substr(1) << ",*," << RASessionID << ",*,*," << RASpecificParams << "\" "
		<< "RCTICKETENCRYPTED=\"0\" "
		<< "DtStart=\"" << ::time(nullptr) << "\" "
		<< "DtLength=\"" << INT32_MAX << "\" "
		<< "L=\"0\""
		<< "/>"
		<< "</UPLOADINFO>\r\n";
	return ticket.str();
}

#endif
