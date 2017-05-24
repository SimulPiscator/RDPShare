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
#ifndef SERVER_H
#define SERVER_H

#include <Winsock2.h>
#include <Windows.h>
#include <string>

class Server
{
public:
	Server(unsigned long ip = 0, unsigned short port = 0);
	~Server();
	DWORD Run(unsigned long ip, unsigned short port);
	std::string Request(const std::string&);

protected:
	void Terminate();
	void AsyncMessage(int, void* = nullptr);
	virtual void OnAsyncMessage(int, void*) {}
	virtual std::string OnRequest(const std::string&) { return ""; }

private:
	struct Private;
	Private* p;
};

#endif // SERVER_H
