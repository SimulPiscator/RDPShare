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
#include <Windows.h>
#include <RdpEncomAPI.h>
#include <Winerror.h>
#include "Sharer.h"
#include "Utils.h"
#include "RDPSessionEvents.h"

struct Sharer::Private
{
	struct Events : RDPSessionEvents
	{
		Sharer* mpSelf;
		Events(Sharer* self) : mpSelf(self) {}
		void OnAttendeeConnected(ComPtr<IRDPSRAPIAttendee> p) override 
		{
			BStr name;
			HR_SucceedOrDie hr = p->get_RemoteName(&name);
			if (name == L"Viewer")
				hr = p->put_ControlLevel(CTRL_LEVEL_VIEW);
			else
				hr = p->put_ControlLevel(CTRL_LEVEL_INTERACTIVE);
			if (mpSelf->p->mAttendees < 0)
				mpSelf->p->mAttendees = 1;
			else
				++mpSelf->p->mAttendees;
		}
		void OnAttendeeDisconnected(ComPtr<IRDPSRAPIAttendee>) override
		{
			--mpSelf->p->mAttendees;
		}
		void OnControlLevelChangeRequest(ComPtr<IRDPSRAPIAttendee> p, CTRL_LEVEL level) override
		{
			HR_SucceedOrDie hr = p->put_ControlLevel(level);
		}
	};
	ComPtr<IRDPSRAPISharingSession> mpSession;
	ComPtr<Events> mpEvents;

	RECT mRect;
	int mPort;
	std::string mConnectionString;
	int mAttendees;
	bool mStarted;

	Private( Sharer* pSelf )
	: mPort(0), mAttendees(-1), mStarted(false)
	{
		::memset(&mRect, 0, sizeof(mRect));
	}
};

Sharer::Sharer()
: p( new Private(this) )
{
}

Sharer::~Sharer()
{
	delete p;
}

Sharer&
Sharer::SetRect(const RECT& r)
{
	p->mRect = r;
	return *this;
}

const RECT&
Sharer::Rect() const
{
	return p->mRect;
}

Sharer&
Sharer::SetPort(int i)
{
	p->mPort = i;
	return *this;
}

int
Sharer::Port() const
{
	return p->mPort;
}

const std::string&
Sharer::ConnectionString() const
{
	return p->mConnectionString;
}

bool
Sharer::LastAttendeeDisconnected() const
{
	return p->mAttendees == 0;
}

Sharer&
Sharer::Start(int maxAttendees)
{
	if (p->mStarted)
		Stop();
	HR_SucceedOrDie hr = ::CoCreateInstance(
		CLSID_RDPSession, nullptr,
		CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
		IID_IRDPSRAPISharingSession, (void**)&p->mpSession
		);
	ComPtr<IConnectionPointContainer> pCPC;
	hr = p->mpSession->QueryInterface(&pCPC);
	ComPtr<IConnectionPoint> pCP;
	hr = pCPC->FindConnectionPoint(DIID__IRDPSessionEvents, &pCP);
	DWORD dwCookie = 0;
	p->mpEvents = ComPtr<Private::Events>(new Private::Events(this));
	hr = pCP->Advise(p->mpEvents, &dwCookie);

	p->mAttendees = -1;
	p->mConnectionString.clear();

	ComPtr<IRDPSRAPISessionProperties> pProperties;
	hr = p->mpSession->get_Properties(&pProperties);
	hr = pProperties->put_Property(BStr(L"DrvConAttach"), Variant(true));
	hr = pProperties->put_Property(BStr(L"PortProtocol"), Variant(AF_INET));
	hr = pProperties->put_Property(BStr(L"PortId"), Variant(p->mPort));

	hr = p->mpSession->put_ColorDepth(24);
	hr = p->mpSession->SetDesktopSharedRect(
		p->mRect.left, p->mRect.top,
		p->mRect.right, p->mRect.bottom
	);

	hr = p->mpSession->Open();
	ComPtr<IRDPSRAPIInvitationManager> pInvitations;
	hr = p->mpSession->get_Invitations(&pInvitations);
	ComPtr<IRDPSRAPIInvitation> pInvitation;
	hr = pInvitations->CreateInvitation(nullptr, BStr(L""), BStr(L""), maxAttendees, &pInvitation);
	BStr connectionString;
	hr = pInvitation->get_ConnectionString(&connectionString);
	p->mConnectionString = connectionString.ToUtf8();
	p->mStarted = true;
	return *this;
}

Sharer&
Sharer::Stop()
{
	if (p->mStarted)
	{
		p->mStarted = false;
		p->mAttendees = 0;
		p->mConnectionString.clear();
		HR_SucceedOrDie hr = p->mpSession->Close();
	}
	return *this;
}

Sharer&
Sharer::Suspend()
{
	HR_SucceedOrDie(p->mpSession->Pause());
	return *this;
}

Sharer&
Sharer::Resume()
{
	HR_SucceedOrDie(p->mpSession->Resume());
	return *this;
}
