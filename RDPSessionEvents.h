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
#ifndef RDP_SESSION_EVENTS_H
#define RDP_SESSION_EVENTS_H

#include <RdpEncomAPI.h>
#include "Utils.h"

class RDPSessionEvents : public IDispatch
{
public:
	RDPSessionEvents() : mRefCount(0) {}
	virtual ~RDPSessionEvents() {}
protected:
	virtual void OnError( HRESULT ) {}

	virtual void OnViewerConnected() {}
	virtual void OnViewerAuthenticated() {}
	virtual void OnViewerConnectFailed() {}
	virtual void OnViewerDisconnected() {}

	virtual void OnSharedDesktopSettingsChanged( int w, int h, int c ) {}
	virtual void OnSharedRectChanged(int, int, int, int) {}

	virtual void OnWindowOpen(ComPtr<IRDPSRAPIWindow>) {}
	virtual void OnWindowClose(ComPtr<IRDPSRAPIWindow>) {}
	virtual void OnWindowUpdate(ComPtr<IRDPSRAPIWindow>) {}

	virtual void OnAttendeeConnected(ComPtr<IRDPSRAPIAttendee>) {}
	virtual void OnAttendeeDisconnected(ComPtr<IRDPSRAPIAttendee>) {}
	virtual void OnAttendeeUpdate(ComPtr<IRDPSRAPIAttendee>) {}
	virtual void OnControlLevelChangeRequest(ComPtr<IRDPSRAPIAttendee>, CTRL_LEVEL) {}

public:
	HRESULT __stdcall QueryInterface( REFIID riid, void** pp) override
	{
		if (!pp)
			return E_POINTER;
		*pp = nullptr;
		if (IsEqualIID(riid, DIID__IRDPSessionEvents))
			*pp = static_cast<IDispatch*>(this);
		if(IsEqualIID(riid, IID_IDispatch))
			*pp = static_cast<IDispatch*>(this);
		if (IsEqualIID(riid, IID_IUnknown))
			*pp = static_cast<IUnknown*>(this);
		if (!(*pp))
			return E_NOINTERFACE;
		this->AddRef();
		return S_OK;
	}
	ULONG __stdcall AddRef() override { ++mRefCount; return 0; }
	ULONG __stdcall Release() override { if (--mRefCount == 0) delete this; return 0; }
	HRESULT __stdcall GetTypeInfoCount( UINT* ) override { return E_NOTIMPL; }
	HRESULT __stdcall GetTypeInfo( UINT, LCID, ITypeInfo** ) override { return E_NOTIMPL; }
	HRESULT __stdcall GetIDsOfNames( REFIID,LPOLESTR*,UINT,LCID ,DISPID* ) override { return E_NOTIMPL; }
	HRESULT __stdcall Invoke( DISPID dispIdMember,REFIID,LCID,WORD,DISPPARAMS* pDispParams,VARIANT*,EXCEPINFO*,UINT* ) override
	{
		VARIANT var = { 0 };
		HRESULT hr = S_OK;
		switch (dispIdMember)
		{
		case DISPID_RDPSRAPI_EVENT_ON_ERROR:
			hr = ::DispGetParam(pDispParams, 0, VT_I4, &var, nullptr);
			OnError( var.intVal );
			break;
		case DISPID_RDPSRAPI_EVENT_ON_VIEWER_CONNECTED:
			OnViewerConnected();
			break;
		case DISPID_RDPSRAPI_EVENT_ON_VIEWER_AUTHENTICATED:
			OnViewerAuthenticated();
			break;
		case DISPID_RDPSRAPI_EVENT_ON_VIEWER_CONNECTFAILED:
			OnViewerConnectFailed();
			break;
		case DISPID_RDPSRAPI_EVENT_ON_VIEWER_DISCONNECTED:
			OnViewerDisconnected();
			break;
		case DISPID_RDPSRAPI_EVENT_ON_SHARED_DESKTOP_SETTINGS_CHANGED:
		{
			long args[3];
			for (int i = 0; i < sizeof(args) / sizeof(*args); ++i)
			{
				hr = ::DispGetParam(pDispParams, i, VT_I4, &var, nullptr);
				args[i] = var.intVal;
			}
			OnSharedDesktopSettingsChanged(args[0], args[1], args[2]);
		}	break;
		case DISPID_RDPSRAPI_EVENT_ON_SHARED_RECT_CHANGED:
		{
			long args[4];
			for (int i = 0; i < sizeof(args) / sizeof(*args); ++i)
			{
				hr = ::DispGetParam(pDispParams, i, VT_I4, &var, nullptr);
				args[i] = var.intVal;
			}
			OnSharedRectChanged(args[0], args[1], args[2], args[3]);
		}	break;
		case DISPID_RDPSRAPI_EVENT_ON_WINDOW_OPEN:
		case DISPID_RDPSRAPI_EVENT_ON_WINDOW_CLOSE:
		case DISPID_RDPSRAPI_EVENT_ON_WINDOW_UPDATE:
			hr = ::DispGetParam(pDispParams, 0, VT_DISPATCH, &var, nullptr);
			if (!FAILED(hr))
			{
				ComPtr<IRDPSRAPIWindow> p;
				hr = var.pdispVal->QueryInterface(&p);
				if (!FAILED(hr)) switch (dispIdMember)
				{
				case DISPID_RDPSRAPI_EVENT_ON_WINDOW_OPEN:
					OnWindowOpen(p);
					break;
				case DISPID_RDPSRAPI_EVENT_ON_WINDOW_CLOSE:
					OnWindowClose(p);
					break;
				case DISPID_RDPSRAPI_EVENT_ON_WINDOW_UPDATE:
					OnWindowUpdate(p);
					break;
				}
			}
			break;
		case DISPID_RDPSRAPI_EVENT_ON_ATTENDEE_CONNECTED:
		case DISPID_RDPSRAPI_EVENT_ON_ATTENDEE_DISCONNECTED:
		case DISPID_RDPSRAPI_EVENT_ON_ATTENDEE_UPDATE:
		case DISPID_RDPSRAPI_EVENT_ON_CTRLLEVEL_CHANGE_REQUEST:
			hr = ::DispGetParam(pDispParams, 0, VT_DISPATCH, &var, nullptr);
			if (!FAILED(hr))
			{
				ComPtr<IRDPSRAPIAttendee> p;
				hr = var.pdispVal->QueryInterface(&p);
				if (!FAILED(hr)) switch (dispIdMember)
				{
				case DISPID_RDPSRAPI_EVENT_ON_ATTENDEE_CONNECTED:
					OnAttendeeConnected(p);
					break;
				case DISPID_RDPSRAPI_EVENT_ON_ATTENDEE_DISCONNECTED:
					OnAttendeeDisconnected(p);
					break;
				case DISPID_RDPSRAPI_EVENT_ON_ATTENDEE_UPDATE:
					OnAttendeeUpdate(p);
					break;
				case DISPID_RDPSRAPI_EVENT_ON_CTRLLEVEL_CHANGE_REQUEST:
					hr = ::DispGetParam(pDispParams, 1, VT_I4, &var, nullptr);
					OnControlLevelChangeRequest(p, CTRL_LEVEL(var.intVal));
					break;
				}
			}
			break;
		default:
			;
		}
		return hr;
	}

private:
	int mRefCount;
};

#endif // RDP_SESSION_EVENTS_H
