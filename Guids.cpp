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

#include <initguid.h>
DEFINE_GUID(CLSID_RDPSession, 0x9B78F0E6, 0x3E05, 0x4A5B, 0xB2, 0xE8, 0xE7, 0x43, 0xA8, 0x95, 0x6B, 0x65);
DEFINE_GUID(DIID__IRDPSessionEvents, 0x98a97042, 0x6698, 0x40e9, 0x8e, 0xfd, 0xb3, 0x20, 0x09, 0x90, 0x00, 0x4b);
DEFINE_GUID(IID_IRDPSRAPISharingSession, 0xeeb20886, 0xe470, 0x4cf6, 0x84, 0x2b, 0x27, 0x39, 0xc0, 0xec, 0x5c, 0xfb);
DEFINE_GUID(IID_IRDPSRAPIAttendee, 0xec0671b3, 0x1b78, 0x4b80, 0xa4, 0x64, 0x91, 0x32, 0x24, 0x75, 0x43, 0xe3);
DEFINE_GUID(IID_IRDPSRAPIAttendeeManager, 0xba3a37e8, 0x33da, 0x4749, 0x8d, 0xa0, 0x07, 0xfa, 0x34, 0xda, 0x79, 0x44);
DEFINE_GUID(IID_IRDPSRAPISessionProperties, 0x339b24f2, 0x9bc0, 0x4f16, 0x9a, 0xac, 0xf1, 0x65, 0x43, 0x3d, 0x13, 0xd4);
DEFINE_GUID(CLSID_RDPSRAPIApplicationFilter, 0xe35ace89, 0xc7e8, 0x427e, 0xa4, 0xf9, 0xb9, 0xda, 0x07, 0x28, 0x26, 0xbd);
DEFINE_GUID(CLSID_RDPSRAPIInvitationManager, 0x53d9c9db, 0x75ab, 0x4271, 0x94, 0x8a, 0x4c, 0x4e, 0xb3, 0x6a, 0x8f, 0x2b);
