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
#ifndef SHARER_H
#define SHARER_H

#include <string>
#include <Windows.h>

class Sharer
{
public:
	Sharer();
	~Sharer();

	Sharer& SetRect(const RECT&);
	const RECT& Rect() const;

	Sharer& SetPort(int);
	int Port() const;

	const std::string& ConnectionString() const;
	bool LastAttendeeDisconnected() const;

	Sharer& Start(int maxAttendees = INT32_MAX );
	Sharer& Stop();
	Sharer& Suspend();
	Sharer& Resume();

private:
	struct Private;
	Private* p;
};

#endif // SHARER_H
