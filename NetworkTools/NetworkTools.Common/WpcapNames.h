/*
 * Copyright (c) 2005-2007 CACE Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of CACE Technologies nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef __WPCAPNAMES_H_EED6D131C6DB4dd696757D219977A7E5
#define __WPCAPNAMES_H_EED6D131C6DB4dd696757D219977A7E5


 //
 // Original names
 //
 //  NOTE: 
 //  - please do not use prefix names longer than 70 chars. 
 //  - the following characters are surely accepted in the prefixes:  "[A-Z][a-z][0-9]_-',"   
 //
#ifdef NPF_NETWORK_TOOLS_RUN_IN_NETWORK_TOOLS_MODE
#define NPF_DRIVER_NAME							"NPF"												///< (HHH) Packet.dll
#define NPF_DRIVER_NAME_WIDECHAR				L"NPF"												///< (HHH) Packet.dll
#define NPF_DRIVER_NAME_SMALL					"npf"												///< (HHH) Packet.dll
#define NPF_DRIVER_NAME_SMALL_WIDECHAR			L"npf"												///< (HHH) Packet.dll
#else
#define NPF_DRIVER_NAME							"NETWORKTOOLS"										///< (HHH) Packet.dll
#define NPF_DRIVER_NAME_WIDECHAR				L"NETWORKTOOLS"										///< (HHH) Packet.dll
#define NPF_DRIVER_NAME_SMALL					"networktools"										///< (HHH) Packet.dll
#define NPF_DRIVER_NAME_SMALL_WIDECHAR			L"networktools"										///< (HHH) Packet.dll
#endif

#define NPF_DRIVER_NAME_SMALL_WIFI				NPF_DRIVER_NAME_SMALL "_wifi"
#define NPF_DRIVER_NAME_SMALL_WIDECHAR_WIFI		NPF_DRIVER_NAME_SMALL_WIDECHAR L"_wifi"

// Used as a logo string in NPFInstall
#define NPF_DRIVER_NAME_NORMAL					"NetworkTools"										///< (HHH) Packet.dll
#define NPF_DRIVER_NAME_NORMAL_WIDECHAR			L"NetworkTools"										///< (HHH) Packet.dll

// Used as the registry software key name
#define NPF_SOFT_REGISTRY_NAME					"NETWORKTOOLS"										///< (HHH) Packet.dll
#define NPF_SOFT_REGISTRY_NAME_WIDECHAR			L"NETWORKTOOLS"										///< (HHH) Packet.dll

#define NPF_ORGAN_NAME							"INSECURE"											///< (HHH) Packet.dll
#define NPF_ORGAN_NAME_WIDECHAR					L"INSECURE"											///< (HHH) Packet.dll

//
// Derived strings
//

// Used in packetWin7\Dll and the driver
#define NPF_DEVICE_NAMES_PREFIX					NPF_DRIVER_NAME "_"     								///< (AAA) packet.dll
#define NPF_DEVICE_NAMES_PREFIX_WIDECHAR		NPF_DRIVER_NAME_WIDECHAR L"_"     						///< (AAA) used by the NPF driver, that does not accept the TEXT(a) macro correctly.
#define NPF_DEVICE_NAMES_PREFIX_WIFI			NPF_DEVICE_NAMES_PREFIX "WIFI_"
#define NPF_DEVICE_NAMES_PREFIX_WIDECHAR_WIFI	NPF_DEVICE_NAMES_PREFIX_WIDECHAR L"WIFI_"

// Only used in packetNtx\Dll
#define NPF_EVENTS_NAMES						NPF_DRIVER_NAME											///< (BBB) 
#define NPF_EVENTS_NAMES_WIDECHAR				NPF_DRIVER_NAME_WIDECHAR								///< (BBB) used by the NPF driver, that does not accept the TEXT(a) macro correctly.

// Used in packetWin7\Dll
#define FAKE_NDISWAN_ADAPTER_NAME				"\\Device\\" NPF_DRIVER_NAME "_GenericDialupAdapter"	///< (CCC) Name of a fake ndiswan adapter that is always available on 2000/XP/2003, used to capture NCP/LCP packets
#define FAKE_NDISWAN_ADAPTER_DESCRIPTION		"Adapter for generic dialup and VPN capture"			///< (DDD) Description of a fake ndiswan adapter that is always available on 2000/XP/2003, used to capture NCP/LCP packets

// Used in packetWin7\Dll, NPFInstall and the driver
#define NPF_SERVICE_DESC						NPF_DRIVER_NAME_NORMAL " Packet Driver (" NPF_DRIVER_NAME ")"						///< (FFF) packet.dll
#define NPF_SERVICE_DESC_WIDECHAR				NPF_DRIVER_NAME_NORMAL_WIDECHAR L" Packet Driver (" NPF_DRIVER_NAME_WIDECHAR L")"	///< (FFF) packet.dll
#define NPF_SERVICE_DESC_TCHAR					_T(NPF_DRIVER_NAME_NORMAL) _T(" Packet Driver (") _T(NPF_DRIVER_NAME) _T(")")		///< (FFF) packet.dll
#define NPF_SERVICE_DESC_WIFI					NPF_SERVICE_DESC " (WiFi version)"
#define NPF_SERVICE_DESC_WIDECHAR_WIFI			NPF_SERVICE_DESC_WIDECHAR L" (WiFi version)"
#define NPF_SERVICE_DESC_TCHAR_WIFI				NPF_SERVICE_DESC_TCHAR _T(" (WiFi version)")

// Used in packetWin7\Dll
#define NPF_DRIVER_COMPLETE_DEVICE_PREFIX		"\\Device\\" NPF_DEVICE_NAMES_PREFIX					///< (III) packet.dll
// Used in packetWin7\Dll
#define NPF_DRIVER_COMPLETE_PATH				"system32\\drivers\\" NPF_DRIVER_NAME ".sys"			///< (LLL) packet.dll


//
// NetworkTools Global Registry Key
//
#define NETWORK_TOOLS_GLOBAL_KEY						"SOFTWARE\\CaceTech\\NetworkToolsOem"
#define NETWORK_TOOLS_GLOBAL_KEY_WIDECHAR				L"SOFTWARE\\CaceTech\\NetworkToolsOem"
#define NETWORK_TOOLS_INSTANCE_KEY					NETWORK_TOOLS_GLOBAL_KEY "\\" NPF_DRIVER_NAME
#define NETWORK_TOOLS_INSTANCE_KEY_WIDECHAR			NETWORK_TOOLS_GLOBAL_KEY_WIDECHAR	L"\\" NPF_DRIVER_NAME_WIDECHAR
#define MAX_NETWORK_TOOLS_KEY_CHARS					512

//
// Subkeys names
//
#define NPF_DEVICES_PREFIX_REG_KEY				"NpfDeviceNamesPrefix"		///< (AAA) 
#define NPF_DEVICES_PREFIX_REG_KEY_WC			L"NpfDeviceNamesPrefix"		///< (AAA) used by the NPF driver, that does not accept the TEXT(a) macro correctly.
#define NPF_EVENTS_NAMES_REG_KEY				"NpfEventsNames"			///< (BBB) 
#define NPF_EVENTS_NAMES_REG_KEY_WC				L"NpfEventsNames"			///< (BBB) used by the NPF driver, that does not accept the TEXT(a) macro correctly.	
#define NPF_FAKE_NDISWAN_ADAPTER_NAME_REG_KEY	"NdiswanAdapterName"		///< (CCC) packet.dll
#define NPF_FAKE_NDISWAN_ADAPTER_DESC_REG_KEY	"NdiswanAdapterDescription"	///< (DDD) packet.dll
#define NPF_SERVICE_DESC_REG_KEY				"NpfServiceDescription"		///< (FFF) packet.dll
#define NPF_DRIVER_NAME_REG_KEY					"NpfDriverName"				///< (HHH) packet.dll
#define NPF_DRIVER_COMPLETE_DEVICE_PREFIX_REG_KEY "NpfCompleteDriverPrefix"	///< (III) packet.dll
#define NPF_DRIVER_COMPLETE_PATH_REG_KEY		"NpfDriverCompletePath"		///< (LLL) 

#endif //__WPCAPNAMES_H_EED6D131C6DB4dd696757D219977A7E5
