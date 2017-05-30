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
#include "Utils.h"
#include <sstream>
#include <Shlwapi.h>

std::string
StringReplace(const std::string& what, const std::string& with, const std::string& in)
{
	std::string s = in;
	for (size_t pos = s.npos; (pos = s.find(what)) != s.npos; )
		s = s.substr(0, pos) + with + s.substr(pos + what.length());
	return s;
}

std::string
GetErrorText(DWORD err)
{
	DWORD winerr = err;
	if (HRESULT_FACILITY(err) == FACILITY_WIN32)
		winerr = HRESULT_CODE(err);
	else if (err & 0xffff0000)
		winerr = 0;
	std::ostringstream oss;
	oss << "Error 0x" << std::hex << err;
	std::string s = oss.str();
	char* text = nullptr;
	if (winerr)
		::FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, winerr, 0, (LPSTR)&text, 0, 0
			);
	if (text)
	{
		s.append(":\n").append(text);
		::HeapFree(::GetProcessHeap(), 0, text);
	}
	return s;
}

std::string GetProgramName()
{
	char buf1[MAX_PATH + 1];
	::GetModuleFileNameA(NULL, buf1, sizeof(buf1));
	std::vector<char> buf2(::GetLongPathNameA(buf1, nullptr, 0));
	::GetLongPathNameA(buf1, buf2.data(), buf2.size());
	const char* begin = ::PathFindFileNameA(buf2.data()), *end = ::PathFindExtensionA(begin);
	return std::string(begin, end - begin);
}

bool
GetScreenRect(unsigned int screen, RECT* pRect)
{
	if (screen == 0)
	{
		pRect->left = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
		pRect->top = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
		pRect->right = pRect->left + ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
		pRect->bottom = pRect->top + ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
		return true;
	}
	struct MonitorEnum
	{
		static BOOL CALLBACK Proc(HMONITOR, HDC, LPRECT pRect, LPARAM pData)
		{
			MonitorEnum* this_ = (MonitorEnum*)pData;
			if (--this_->count == 0)
			{
				*this_->pRect = *pRect;
				return FALSE;
			}
			return TRUE;
		}
		unsigned int count;
		RECT* pRect;
	} monitorEnum = { screen, pRect };
	::EnumDisplayMonitors(NULL, nullptr, &MonitorEnum::Proc, (LPARAM)&monitorEnum);
	return monitorEnum.count == 0;
}

int
SetScreenSize(unsigned int screen, int width, int height)
{
	if (screen == 0)
		return false;
	struct MonitorEnum
	{
		static BOOL CALLBACK Proc(HMONITOR hMonitor, HDC, LPRECT, LPARAM pData)
		{
			MonitorEnum* this_ = (MonitorEnum*)pData;
			if (--this_->count == 0)
			{
				MONITORINFOEXA info = {};
				info.cbSize = sizeof(MONITORINFOEXA);
				if (::GetMonitorInfoA(hMonitor, &info))
				{
					DEVMODEA mode = {};
					mode.dmSize = sizeof(DEVMODEA);
					if(::EnumDisplaySettingsA(info.szDevice, ENUM_CURRENT_SETTINGS, &mode))
					{
						mode.dmPelsWidth = this_->width;
						mode.dmPelsHeight = this_->height;
						this_->result = ::ChangeDisplaySettingsExA(info.szDevice, &mode, NULL, 0, NULL);
					}
				}
				return FALSE;
			}
			return TRUE;
		}
		unsigned int count;
		int width, height;
		int result;
	} monitorEnum = { screen, width, height, DISP_CHANGE_BADPARAM };
	::EnumDisplayMonitors(NULL, nullptr, &MonitorEnum::Proc, (LPARAM)&monitorEnum);
	return monitorEnum.result;
}

std::string UrlEscape_(const std::string& in, const std::string& allow)
{
	std::string out;
	for (auto c : in)
	{
		if (::isalnum(c) || allow.find(c) != allow.npos)
			out += c;
		else
		{
			out += '%';
			std::ostringstream oss;
			oss.width(2);
			oss.fill('0');
			oss << std::hex << int(c);
			out += oss.str();
		}
	}
	return out;
}

std::istream&
QuotedString::Unserialize( std::istream& is )
{
	std::string& self = *static_cast<std::string*>(this);
	is >> std::ws;
	if (is.peek() == '"')
		return std::getline(is.ignore(), self, '"');
	return is >> self;
}
