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
#ifndef UTILS_H
#define UTILS_H

#include <Windows.h>
#include <string>
#include <vector>

struct WSA_SucceedOrDie
{
	WSA_SucceedOrDie(int r = 0) : r(r) { if (r<0) throw *this; }
	operator int() const { return r; }
	int r;
};

struct HR_SucceedOrDie
{
	HR_SucceedOrDie(HRESULT hr = 0) : hr( hr ) { if (FAILED(hr)) throw *this; }
	operator HRESULT() { return hr; }
	HRESULT hr;
};

struct BOOL_SucceedOrDie
{
	BOOL_SucceedOrDie(BOOL b = TRUE) : b(b) { if (!b) { error = ::GetLastError();  throw *this; } }
	operator BOOL() { return b; }
	BOOL b;
	DWORD error;
};

template<class T> class ComPtr
{
public:
	ComPtr() : p( nullptr ) {}
	ComPtr(const ComPtr& other) : p(nullptr) { operator=(other); }
	explicit ComPtr(T* p) : p(p) { AddRef(); }
	~ComPtr() { Release(); }
	void Reset() { Release(); p = nullptr; }
	ComPtr& operator=(const ComPtr& other) { other.AddRef(); Release(); p = other.p; return *this; }
	T** operator&() { Reset(); return &p; }
	T* operator->() { return p; }
	const T* operator->() const { return p; }
	operator T*() const { return p; }
	operator const T*() { return p; }
private:
	void AddRef() const { if (p) p->AddRef(); }
	void Release() { if (p) p->Release(); }
	T* p;
};

class BStr
{
public:
	BStr() : data(nullptr) {}
	BStr(const wchar_t* str) : data(::SysAllocString(str)) {}
	BStr(const std::string& s) :data(nullptr) { FromUtf8(s); }
	BStr& operator=(const std::string& s) { return FromUtf8(s); }
	~BStr() { ::SysFreeString(data); }
	operator BSTR() { return data; }
	BSTR* operator&() { ::SysFreeString(data); data = nullptr; return &data; }
	bool operator==(const wchar_t* p) const { return std::wstring(p) == data; }
	std::string ToUtf8() const
	{
		std::string s;
		if (data)
		{
			s.resize(::WideCharToMultiByte(CP_UTF8, 0, data, -1, 0, 0, 0,0));
			::WideCharToMultiByte(CP_UTF8, 0, data, -1, const_cast<char*>(s.data()), s.size(), 0, 0);
			if(s.size()>0)
				s.resize(s.size() - 1);
		}
		return s;
	}
	BStr& FromUtf8(const std::string& s)
	{
		::SysFreeString(data);
		std::vector<wchar_t> buf(::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, 0, 0));
		::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, buf.data(), buf.size());
		data = ::SysAllocString(buf.data());
		return *this;
	}

private:
	BSTR data;
};

struct Variant : VARIANT
{
	Variant(bool b) { VARIANT::boolVal = b ? VARIANT_TRUE : VARIANT_FALSE; VARIANT::vt = VT_BOOL; }
	Variant(int i) { VARIANT::intVal = i; VARIANT::vt = VT_I4; }
};

struct QuotedString : std::string
{
	std::istream& Unserialize(std::istream&);
	std::ostream& Serialize(std::ostream&) const;
};

inline std::istream& operator>>(std::istream& is, QuotedString& s) { return s.Unserialize(is); }
inline std::ostream& operator<<(std::ostream& os, const QuotedString& s) { return s.Serialize(os); }



std::string StringReplace(const std::string& what, const std::string& with, const std::string&);
std::string GetErrorText(DWORD);
std::string GetProgramName();
bool GetScreenRect(unsigned int screen, RECT* pRect);
int SetScreenSize(unsigned int screen, int width, int height);
std::string UrlEscape_(const std::string& in, const std::string& allow = "");

#endif // UTILS_H
