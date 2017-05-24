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

#include <Winsock2.h>
#include <Windows.h>
#include <process.h>
#include "Utils.h"
#include <mutex>

static struct WSAInit
{
	WSAInit()
	{
		WSADATA ignored;
		::WSAStartup(MAKEWORD(2, 2), &ignored);
	}
	~WSAInit()
	{
		::WSACleanup();
	}
} sWSAInit;

struct Server::Private
{
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		Server* this_ = (Server*)::GetWindowLongPtrA(hwnd, GWLP_USERDATA);
		switch (msg)
		{
		case WM_CREATE:
			::SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lparam)->lpCreateParams);
			return 0;
		case WM_USER:
			this_->OnAsyncMessage(int(wparam), (void*)lparam);
			return 0;
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	Private( Server* pSelf)
		: mpSelf(pSelf), mMessageThread(0), mServerThread(0), mListeningSocket(-1)
	{
		WNDCLASSA c = { 0 };
		c.lpfnWndProc = &WindowProc;
		c.hInstance = ::GetModuleHandleA(nullptr);
		c.lpszClassName = "ServerMessageWindow";
		::RegisterClassA(&c);
		mHwnd = ::CreateWindowA(c.lpszClassName, 0, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, c.hInstance, mpSelf);
	}
	~Private()
	{
	}
	void InitAddress(unsigned long ip, unsigned short port);
	static void Send(SOCKET s, const std::string&);
	static std::string Receive(SOCKET s);
	static unsigned int WINAPI ThreadProc(LPVOID);
	unsigned int WaitForServerThread();

	Server* mpSelf;
	std::mutex mMutex;
	HWND mHwnd;
	sockaddr_in mAddress;
	unsigned int mMessageThread;
	HANDLE mServerThread;
	SOCKET mListeningSocket;
};

Server::Server(unsigned long ip, unsigned short port)
	:  p(new Private(this))
{
	p->InitAddress(ip, port);
}

Server::~Server()
{
	Terminate();
	p->WaitForServerThread();
	delete p;
}

void
Server::Private::InitAddress(unsigned long ip, unsigned short port)
{
	::memset(&mAddress, 0, sizeof(mAddress));
	mAddress.sin_family = AF_INET;
	mAddress.sin_addr.s_addr = ip;
	mAddress.sin_port = htons(port);
}

DWORD
Server::Run(unsigned long ip, unsigned short port)
{
	p->mMessageThread = ::GetCurrentThreadId();
	p->InitAddress(ip, port);

	WSA_SucceedOrDie r;
	SOCKET ls = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	r = ::bind(ls, (sockaddr*)&p->mAddress, sizeof(p->mAddress));
	r = ::listen(ls, SOMAXCONN);
	p->mListeningSocket = ls;

	p->mServerThread = (HANDLE)::_beginthreadex(nullptr, 0, &Private::ThreadProc, p, 0, nullptr);
	BOOL_SucceedOrDie b = p->mServerThread != 0;
	MSG msg;
	while (::GetMessageA(&msg, NULL, 0, 0) > 0)
	{
		std::lock_guard<std::mutex> lock(p->mMutex);
		::TranslateMessage(&msg);
		::DispatchMessageA(&msg);
	}
	Terminate();
	return p->WaitForServerThread();
}

unsigned int
Server::Private::WaitForServerThread()
{
	DWORD exitCode = 0;
    HANDLE hThread = ::InterlockedExchangePointer(&mServerThread, 0);
	if (hThread)
	{
		BOOL_SucceedOrDie b = BOOL(WAIT_OBJECT_0 == ::WaitForSingleObject(hThread, INFINITE));
		b = ::GetExitCodeThread(hThread, &exitCode);
		::CloseHandle(hThread);
	}
	return exitCode;
}

void
Server::Terminate()
{
	SOCKET s = ::InterlockedExchange(&p->mListeningSocket,-1);
	if ( s != -1)
		WSA_SucceedOrDie r = ::closesocket(s);
}

void
Server::Private::Send(SOCKET s, const std::string& data)
{
	int remaining = data.length();
	const char* p = data.c_str();
	while (remaining > 0)
	{
		fd_set writefds;
		FD_ZERO(&writefds);
		FD_SET(s, &writefds);
		timeval t = { 1, 0 };
		WSA_SucceedOrDie r = ::select(s + 1, nullptr, &writefds, nullptr, &t);
		if (FD_ISSET(s, &writefds))
			r = ::send(s, p, remaining, 0);
		else
			r = remaining;
		p += r;
		remaining -= r;
	}
}

std::string
Server::Private::Receive(SOCKET s)
{
	std::string data;
	char c = -1;
	while (c)
	{
		c = 0;
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(s, &readfds);
		timeval t = { 20, 0 };
		WSA_SucceedOrDie r = ::select(s + 1, &readfds, nullptr, nullptr, &t);
		if (FD_ISSET(s, &readfds))
			r = ::recv(s, &c, 1, 0);
		if (c == '\n')
			c = 0;
		if (c && c != '\r')
			data += c;
	}
	return data;
}

unsigned int WINAPI
Server::Private::ThreadProc(LPVOID data)
{
	Server::Private* this_ = static_cast<Server::Private*>(data);
	unsigned int result = 0;
	try
	{
		while (this_->mListeningSocket != -1)
		{
			SOCKET s = ::accept(this_->mListeningSocket, nullptr, nullptr);
			if (s != -1)
			{
				std::lock_guard<std::mutex> lock(this_->mMutex);
				std::string request = Receive(s);
				Send(s, this_->mpSelf->OnRequest(request));
				WSA_SucceedOrDie r = ::closesocket(s);
			}
		}
	}
	catch( WSA_SucceedOrDie)
	{
		result = ::WSAGetLastError();
	}
	catch (BOOL_SucceedOrDie)
	{
		result = ::GetLastError();
	}
	::PostThreadMessageA(this_->mMessageThread, WM_QUIT, 0, 0);
	return result;
}

std::string
Server::Request( const std::string& command )
{
	SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	WSA_SucceedOrDie r = ::connect(s, (sockaddr*)&p->mAddress, sizeof(p->mAddress));
	Private::Send(s,command + "\r\n");
	std::string data = Private::Receive(s);
	r = ::closesocket(s);
	return data;
}

void
Server::AsyncMessage(int msg, void* arg)
{
	BOOL_SucceedOrDie b = ::PostMessageA(p->mHwnd, WM_USER, msg, LPARAM(arg));
}
