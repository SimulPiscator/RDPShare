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
#ifndef RDP_FILE_H
#define RDP_FILE_H

#include<string>

struct RemoteAssistanceInfo
{
	std::string IP, Port, SessionID;
};

bool ParseConnectionString(const std::string&, RemoteAssistanceInfo&);
std::string CreateTicket(const std::string& connectionString);
std::string CreateRdpFile(const std::string&, int, int, bool freerdpworkaround);
std::string RdpFileToUri(const std::string&);

#endif // RDP_FILE_H
