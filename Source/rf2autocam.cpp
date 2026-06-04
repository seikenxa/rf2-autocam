//ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
//ï¿½                                                                         ï¿½
//ï¿½ Module: Internals Example Source File                                   ï¿½
//ï¿½                                                                         ï¿½
//ï¿½ Description: Declarations for the Internals Example Plugin              ï¿½
//ï¿½                                                                         ï¿½
//ï¿½                                                                         ï¿½
//ï¿½ This source code module, and all information, data, and algorithms      ï¿½
//ï¿½ associated with it, are part of CUBE technology (tm).                   ï¿½
//ï¿½                 PROPRIETARY AND CONFIDENTIAL                            ï¿½
//ï¿½ Copyright (c) 1996-2014 Image Space Incorporated.  All rights reserved. ï¿½
//ï¿½                                                                         ï¿½
//ï¿½                                                                         ï¿½
//ï¿½ Change history:                                                         ï¿½
//ï¿½   tag.2005.11.30: created                                               ï¿½
//ï¿½                                                                         ï¿½
//ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½

#include "rf2autocam.hpp"          // corresponding header file
#include <math.h>               // for atan2, sqrt
#include <limits>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>            // for std::transform (case-insensitive compare)
#include <process.h>
#include <thread>               // for SwitchCameraViaREST background thread
#include <winhttp.h>            // for LMU REST API calls
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

#include <ctime>

// Case-insensitive string comparison helper (replaces _stricmp for std::string)
static bool iequals(const std::string& a, const std::string& b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (tolower(static_cast<unsigned char>(a[i])) != tolower(static_cast<unsigned char>(b[i])))
            return false;
    return true;
}

// LMU camera switch: PUT http://localhost:6397/rest/watch/focus/{slotId}
// Runs in a detached background thread to avoid blocking the game loop.
void rF2autocam::SwitchCameraViaREST(int slotId)
{
    std::thread([slotId]() {
        HINTERNET hSession = WinHttpOpen(L"rF2AutoCam/1.0",
            WINHTTP_ACCESS_TYPE_NO_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return;

        HINTERNET hConnect = WinHttpConnect(hSession, L"localhost", 6397, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return; }

        std::wstring path = L"/rest/watch/focus/" + std::to_wstring(slotId);
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"PUT", path.c_str(),
            NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }

        DWORD timeout = 3000; // 3 sec
        WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SEND_TIMEOUT,    &timeout, sizeof(timeout));
        WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

        WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        WinHttpReceiveResponse(hRequest, NULL);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
    }).detach();
}

// Read the sim's own Instant Replay key binding so manual (live-cut) mode follows
// whatever key the user has configured. rF2 stores it in UserData\player\Controller.JSON
// as  "Control - Instant Replay":[0, <scancode>] ; LMU's keyboard.json uses a flat
// "Instant Replay": <scancode>. Both are DirectInput scancodes (e.g. 19 = R).
// Returns a Win32 virtual-key code (VK_R = 0x52 by default if nothing is found).
int rF2autocam::DetectReplayKeyVK()
{
    const size_t slash = inifilename.find_last_of("/\\");
    const std::string dir = (slash == std::string::npos) ? "." : inifilename.substr(0, slash);
    const char* files[] = { "\\Controller.JSON", "\\keyboard.json" };
    for (const char* fn : files) {
        std::ifstream f(dir + fn);
        if (!f.is_open()) continue;
        std::stringstream ss; ss << f.rdbuf();
        const std::string content = ss.str();
        const size_t p = content.find("Instant Replay");
        if (p == std::string::npos) continue;
        const size_t colon = content.find(':', p);
        if (colon == std::string::npos) continue;
        const size_t s = content.find_first_not_of(" \t\r\n", colon + 1);
        if (s == std::string::npos) continue;
        std::string seg;
        if (content[s] == '[') {                         // rF2 array form: [0, 19]
            const size_t e = content.find(']', s);
            seg = content.substr(s, (e == std::string::npos) ? 40 : e - s);
        } else {                                         // LMU scalar form: 19
            const size_t e = content.find_first_of(",\r\n}", s);
            seg = content.substr(s, (e == std::string::npos) ? 8 : e - s);
        }
        int sc = -1;                                     // last integer token in seg
        for (size_t i = 0; i < seg.size(); ) {
            if (isdigit(static_cast<unsigned char>(seg[i]))) {
                sc = atoi(seg.c_str() + i);
                while (i < seg.size() && isdigit(static_cast<unsigned char>(seg[i]))) ++i;
            } else ++i;
        }
        if (sc > 0) {
            const UINT vk = MapVirtualKeyA(static_cast<UINT>(sc), MAPVK_VSC_TO_VK);
            if (vk != 0) return static_cast<int>(vk);
        }
    }
    return 0x52; // VK_R
}

// plugin information

extern "C" __declspec( dllexport )
const char * __cdecl GetPluginName()                   { return( "rF2 autocam - 2026.05.29." ); }

extern "C" __declspec( dllexport )
PluginObjectType __cdecl GetPluginType()               { return( PO_INTERNALS ); }

extern "C" __declspec( dllexport )
int __cdecl GetPluginVersion()                         { return( 7 ); } // InternalsPluginV01 functionality (if you change this return value, you must derive from the appropriate class!)

extern "C" __declspec( dllexport )
PluginObject * __cdecl CreatePluginObject()            { return((PluginObject *) new rF2autocam); }

extern "C" __declspec( dllexport )
void __cdecl DestroyPluginObject(PluginObject *obj)  { delete((rF2autocam *)obj); }


// ExampleInternalsPlugin class

void rF2autocam::ResetSessionState()
{
    camvalttime   = 0.0;
    needpos       = 0;
    needspos      = 0;
    needdpos      = 0;
    needveh       = 0;
    aktveh        = -1;
    playerSlotId  = -1;
    aktpos        = 0;
    shownCount    = 0;
    lastLeader    = -1;
    timerFired    = false;
    needcam       = kCamTrackside;
    lastcam       = 0;
    refreshcount  = 0;
    bestlapT      = kNoLapTime;
    best1T        = kNoLapTime;
    best2T        = kNoLapTime;
    inpit         = false;
    sbs           = 0;
    maxsbs        = 0;
    needreplay    = false;
    onreplay      = false;
    stopreplay    = false;
    replaystarted = 0.0;
    replayset     = false;
    replayveh     = -1;
    incidentActive   = false;
    incidentSince    = 0.0;
    replaykeypressed = false;
    livecutFocus     = false;
    livecutFocusStart = 0.0;
    inctime       = 0.0;
    incsize       = 0.0;
    preplayveh    = -1;
    pinctime      = 0.0;
    pincsize      = 0.0;
    completedlaps = 0;
    currentlap    = 0;
    aktname.clear();
    elso.clear();
    prevResultsStream.clear();
    prevResultsReady = false;
}

void rF2autocam::WritetoFileDrivername()
{
	std::ofstream fdriver(driverfname);
	if (fdriver.is_open())
	{
		if (onreplay)
			fdriver << replayname;
		else
			fdriver << aktpos << ". " << aktname;
	}
}

void rF2autocam::WritetoInfohtml(long session)
{
	std::ofstream flist(listfname);
	if (!flist.is_open()) return;

	flist <<
		"<!DOCTYPE html>"
		"<html><head>"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
		"<meta http-equiv=\"refresh\" content=\"0.5\">"
		"<style>"
		"body { font-size:12pt; font-family:Sans-serif; font-weight:bold; }"
		"tr { background-size:100% 100%; }"
		"td.head { background-color:lightgray; color:black; text-align:center; }"
		"td.value { background-color:black; color:white; text-align:right; }"
		"</style></head><body>";

	if (!onreplay && session < 10)
	{
		char buf[128];
		flist << "<table>";
		flist << "<tr><td class=\"head\" width=\"45px\">Best:</td>";
		snprintf(buf, sizeof(buf), "<td class=\"value\" width=\"65px\">%.3f</td>", best1T);        flist << buf;
		snprintf(buf, sizeof(buf), "<td class=\"value\" width=\"65px\">%.3f</td>", best2T - best1T); flist << buf;
		snprintf(buf, sizeof(buf), "<td class=\"value\" width=\"65px\">%.3f</td>", bestlapT - best2T); flist << buf;
		snprintf(buf, sizeof(buf), "<td class=\"value\" width=\"80px\">%.0f:%06.3f</td>", floor(bestlapT / 60), fmod(bestlapT, 60)); flist << buf;
		flist << "</tr>";
		flist << "<tr><td class=\"head\" width=\"45px\">Dif:</td>"
		         "<td class=\"value\" width=\"65px\">-</td>"
		         "<td class=\"value\" width=\"65px\">-</td>"
		         "<td class=\"value\" width=\"65px\">-</td>"
		         "<td class=\"value\" width=\"80px\">-</td></tr>";
		flist << "<tr><td class=\"head\" width=\"45px\">Curr:</td>"
		         "<td class=\"value\" width=\"65px\">199.999</td>"
		         "<td class=\"value\" width=\"65px\">199.999</td>"
		         "<td class=\"value\" width=\"65px\">199.999</td>";
		snprintf(buf, sizeof(buf), "<td class=\"value\" width=\"80px\">%.0f:%06.3f</td>", floor(curlapT / 60), fmod(curlapT, 60)); flist << buf;
		flist << "</tr></table>";
	}

	flist << "</body></html>";
}

void rF2autocam::WriteToJson(long session, const std::string& timestr)
{
	std::ofstream f(jsonfname);
	if (!f.is_open()) return;

	// Camera type → string
	const char* camname = "trackside";
	if      (needcam == 0)    camname = "tvcockpit";
	else if (needcam == 1)    camname = "cockpit";
	else if (needcam == 2)    camname = "nosecam";
	else if (needcam == 3)    camname = "swingman";
	else if (needcam == rvcam && needcam != kCamTrackside) camname = "rearview";
	else if (needcam >= 5)    camname = "onboard";

	// Session type → string
	const char* sessname = "practice";
	if      (session >= 10)              sessname = "race";
	else if (session == 4 || session == 5 ||
	         session == 6 || session == 7 ||
	         session == 8 || session == 9) sessname = "qualifying";

	// Escape helper (driver names may contain special chars)
	auto jsonEscape = [](const std::string& s) {
		std::string out;
		out.reserve(s.size());
		for (char c : s) {
			if      (c == '"')  out += "\\\"";
			else if (c == '\\') out += "\\\\";
			else                out += c;
		}
		return out;
	};

	// Game phase → string
	const char* phasename = "green";
	switch (gamePhase) {
		case 0:           phasename = "garage";    break;
		case 1: case 2:   phasename = "warmup";    break;
		case 3:           phasename = "formation"; break;
		case 4: case 5:   phasename = "green";     break;
		case 6:           phasename = "yellow";    break;
		case 7:           phasename = "stopped";   break;
		case 8:           phasename = "finished";  break;
		default:          phasename = "unknown";   break;
	}

	const bool inBattle   = (pontosminbehind >= 0.05) && (pontosminbehind <= obtime);
	const bool sbsActive  = (maxsbs >= sbscount);

	f << "{\n";
	if (onreplay) {
		f << "  \"driver\": \""   << jsonEscape(replayname) << "\",\n";
		f << "  \"position\": 0,\n";
	} else {
		f << "  \"driver\": \""   << jsonEscape(aktname) << "\",\n";
		f << "  \"position\": "   << aktpos << ",\n";
	}
	f << "  \"camera\": \""       << camname  << "\",\n";
	f << "  \"on_replay\": "        << (onreplay       ? "true" : "false") << ",\n";
	f << "  \"incident\": "         << (incidentActive ? "true" : "false") << ",\n";
	f << "  \"autocam\": "          << (automatic     ? "true" : "false") << ",\n";
	f << "  \"player_driving\": "   << (playerDriving ? "true" : "false") << ",\n";
	f << "  \"session_type\": \""   << sessname << "\",\n";
	f << "  \"game_phase\": \""   << phasename << "\",\n";
	f << "  \"time_display\": \"" << jsonEscape(timestr) << "\",\n";
	f << "  \"gap_to_next\": "
	  << std::fixed << std::setprecision(3) << pontosminbehind << ",\n";
	f << "  \"in_battle\": "      << (inBattle  ? "true" : "false") << ",\n";
	f << "  \"sbs_active\": "     << (sbsActive ? "true" : "false") << ",\n";
	f << "  \"leader\": \""       << jsonEscape(elso) << "\"\n";
	f << "}\n";
}

void rF2autocam::SetEnvironment(const EnvironmentInfoV01 &info)
{
	char seged[256] = {};
	char* e = nullptr;
	// as the SetEnvironment can be called several times at launch                  
	/* if (environmentAlreadySet) {
	return;
	} */
	// retrieve the ini filename (full path)
	std::string str = info.mPath[1];
	size_t found = str.find_last_of("/\\");
	str = str.substr(0, found);
	str.append("\\rF2autocam.ini");
	inifilename = str;
	GetPrivateProfileString("AUTOCAM", "auto", "a", seged, 255, str.c_str());
	automatic = strtol(seged, &e, 0);
	if (0 == waitsec && seged == e) {
		automatic = 1;
		WritePrivateProfileString("AUTOCAM", "auto", "1", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "autokey", "a", seged, 255, str.c_str());
	autokey = strtol(seged, &e, 0);
	if (0 == autokey && seged == e) {
		autokey = 0x41;  // default: A (VK_A) — matches h0rcs4 original default
		WritePrivateProfileString("AUTOCAM", "autokey", "0x41", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "waitsec", "a", seged, 255, str.c_str());
	waitsec = strtol(seged, &e, 0);
	if (0 == waitsec && seged == e) {
		waitsec = 15;
		WritePrivateProfileString("AUTOCAM", "waitsec", "15", str.c_str());
	}	
	GetPrivateProfileString("AUTOCAM", "interest", "a", seged, 255, str.c_str());
	interest = strtol(seged, &e, 0);
	if (0 == interest && seged == e) {
		interest = 12;		
		WritePrivateProfileString("AUTOCAM", "interest", "12", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "interestdiff", "a", seged, 255, str.c_str());
	interestsec = strtol(seged, &e, 0);
	if (0 == interestsec && seged == e) {
		interestsec = 3;
		WritePrivateProfileString("AUTOCAM", "interestdiff", "3", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "onboarddiff", "a", seged, 255, str.c_str());
	obtime = strtod(seged, &e);
	if (0 == obtime && seged == e) {
		obtime = 0.4;
		WritePrivateProfileString("AUTOCAM", "onboarddiff", "0.4", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "onboardcam", "a", seged, 255, str.c_str());
	obcam = strtol(seged, &e, 0);
	if (0 == obcam && seged == e) {
		obcam = 0;
		WritePrivateProfileString("AUTOCAM", "onboardcam", "0", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "rearview", "a", seged, 255, str.c_str());
	rearview = strtol(seged, &e, 0);
	if (0 == rearview && seged == e) {
		rearview = 0;
		WritePrivateProfileString("AUTOCAM", "rearview", "0", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "rearviewcam", "a", seged, 255, str.c_str());
	rvcam = strtol(seged, &e, 0);
	if (0 == rvcam && seged == e) {
		rvcam = 6;
		WritePrivateProfileString("AUTOCAM", "rearviewcam", "6", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "camtest", "0", seged, sizeof(seged), str.c_str());
	camtest = seged;
	if (iequals(camtest, "0")) {
		camtest = "no";
		WritePrivateProfileString("AUTOCAM", "camtest", "no", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "walkthrough", "a", seged, sizeof(seged), str.c_str());
	walkthrough = strtol(seged, &e, 0);
	if (0 == walkthrough && seged == e) {
		walkthrough = 1;
		WritePrivateProfileString("AUTOCAM", "walkthrough", "1", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "showinpit", "0", seged, sizeof(seged), str.c_str());
	showinpit = seged;
	if (iequals(showinpit, "0")) {
		showinpit = "interestdiff";
		WritePrivateProfileString("AUTOCAM", "showinpit", "interestdiff", str.c_str());
	}
	showinpitl = strtol(showinpit.c_str(), &e, 0);
	GetPrivateProfileString("AUTOCAM", "lowinc", "a", seged, sizeof(seged), str.c_str());
	lowinc = strtod(seged, &e);
	if (0 == lowinc && seged == e) {
		lowinc = 500.0;
		WritePrivateProfileString("AUTOCAM", "lowinc", "500", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "highinc", "a", seged, sizeof(seged), str.c_str());
	highinc = strtod(seged, &e);
	if (0 == highinc && seged == e) {
		highinc = 2000.0;
		WritePrivateProfileString("AUTOCAM", "highinc", "2000", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "filespath", "0", seged, sizeof(seged), str.c_str());
	filespath = seged;
	if (iequals(filespath, "0")) {
		// Default: rF2stream folder next to the ini file (avoids hardcoded absolute path)
		filespath = inifilename.substr(0, inifilename.find_last_of("/\\")) + "\\rF2stream";
		WritePrivateProfileString("AUTOCAM", "filespath", filespath.c_str(), str.c_str());
	}
	// Create output directory if it doesn't exist (silent if already exists)
	CreateDirectoryA(filespath.c_str(), NULL);
	GetPrivateProfileString("AUTOCAM", "debug", "a", seged, sizeof(seged), str.c_str());
	debuglog = strtol(seged, &e, 0);
	if (0 == debuglog && seged == e) {
		debuglog = 0;
		WritePrivateProfileString("AUTOCAM", "debug", "0", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "sbsdist", "a", seged, sizeof(seged), str.c_str());
	sbsdist = strtod(seged, &e);
	if (0 == sbsdist && seged == e) {
		sbsdist = 1.5;
		WritePrivateProfileString("AUTOCAM", "sbsdist", "1.5", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "sbscount", "a", seged, sizeof(seged), str.c_str());
	sbscount = strtol(seged, &e, 0);
	if (0 == sbscount && seged == e) {
		sbscount = 2;
		WritePrivateProfileString("AUTOCAM", "sbscount", "2", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "replayduration", "a", seged, sizeof(seged), str.c_str());
	replayduration = strtol(seged, &e, 0);
	if (0 == replayduration && seged == e) {
		replayduration = 20;
		WritePrivateProfileString("AUTOCAM", "replayduration", "20", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "replayoffset", "a", seged, sizeof(seged), str.c_str());
	replayoffset = strtod(seged, &e);
	if (0 == replayoffset && seged == e) {
		replayoffset = 5.0;
		WritePrivateProfileString("AUTOCAM", "replayoffset", "5.0", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "livecut", "a", seged, sizeof(seged), str.c_str());
	livecut = strtol(seged, &e, 0);
	if (0 == livecut && seged == e) {
		livecut = 0;
		WritePrivateProfileString("AUTOCAM", "livecut", "0", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "incidenthold", "a", seged, sizeof(seged), str.c_str());
	incidenthold = strtol(seged, &e, 0);
	if (0 == incidenthold && seged == e) {
		incidenthold = 7;
		WritePrivateProfileString("AUTOCAM", "incidenthold", "7", str.c_str());
	}
	// replaykey: "auto" (default) reads the sim's own Instant Replay binding;
	// an explicit hex VK (e.g. 0x52) overrides it for edge cases (wheel button, etc.)
	GetPrivateProfileString("AUTOCAM", "replaykey", "0", seged, sizeof(seged), str.c_str());
	{
		const std::string rk = seged;
		if (iequals(rk, "0") || iequals(rk, "auto")) {
			replaykey = DetectReplayKeyVK();
			WritePrivateProfileString("AUTOCAM", "replaykey", "auto", str.c_str());
		} else {
			const int v = strtol(seged, &e, 0);
			replaykey = (v > 0) ? v : DetectReplayKeyVK();
		}
	}
	// LMU detection (once only): check if REST API is running on localhost:6397
	if (!lmuDetected) {
		lmuDetected = true;
		isLMU = false;
		HINTERNET hSession = WinHttpOpen(L"rF2AutoCam/1.0",
			WINHTTP_ACCESS_TYPE_NO_PROXY,
			WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (hSession) {
			HINTERNET hConnect = WinHttpConnect(hSession, L"localhost", 6397, 0);
			if (hConnect) {
				HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET",
					L"/rest/watch/sessionInfo", NULL, WINHTTP_NO_REFERER,
					WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
				if (hRequest) {
					DWORD timeout = 2000; // 2 sec timeout to avoid startup delay
					WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
					WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
					if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
						WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
						if (WinHttpReceiveResponse(hRequest, NULL)) {
							DWORD code = 0, sz = sizeof(code);
							WinHttpQueryHeaders(hRequest,
								WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
								NULL, &code, &sz, NULL);
							isLMU = (code == 200);
						}
					}
					WinHttpCloseHandle(hRequest);
				}
				WinHttpCloseHandle(hConnect);
			}
			WinHttpCloseHandle(hSession);
		}
	}
	// environmentAlreadySet = true;
}

void rF2autocam::Startup( long version )
{
    srand(static_cast<unsigned int>(time(NULL)));

    // default HW control enabled to true
    mEnabled = true;
    message.mDestination = 0;
    message.mTranslate = 0;
    if (automatic) {
        strcpy(message.mText, "rF2autocam 2026.05.29. Auto camera: on");
    } else {
        strcpy(message.mText, "rF2autocam 2026.05.29. Auto camera: off");
    }

    // out files defaults (must be set before ResetSessionState writes them)
    timefname   = filespath + "\\time.txt";
    driverfname = filespath + "\\driver.txt";
    listfname   = filespath + "\\info.html";
    jsonfname   = filespath + "\\status.json";

    ResetSessionState();

    { std::ofstream f(timefname);   if (f.is_open()) f << "-"; }
    { std::ofstream f(driverfname); if (f.is_open()) f << "rF2autocam"; }
    { std::ofstream f(listfname);   } // create/truncate
    { std::ofstream f(jsonfname);   } // create/truncate
    WritetoInfohtml(0);
}


void rF2autocam::Shutdown()
{
  
}


void rF2autocam::StartSession()
{
    ResetSessionState();

    { std::ofstream f(timefname);   if (f.is_open()) f << "-"; }
    { std::ofstream f(driverfname); if (f.is_open()) f << "rF2autocam"; }
    { std::ofstream f(listfname);   } // create/truncate
    { std::ofstream f(jsonfname);   } // create/truncate
    WritetoInfohtml(0);
}


void rF2autocam::EndSession()
{
  
}


void rF2autocam::EnterRealtime()
{
  // start up timer every time we enter realtime
  mET = 0.0;
  
}


void rF2autocam::ExitRealtime()
{
  
}


void rF2autocam::UpdateTelemetry( const TelemInfoV01 &info )
{
}


void rF2autocam::UpdateGraphics( const GraphicsInfoV01 &info )
{
  // Use the incoming data, for now I'll just write some of it to a file to a) make sure it
  // is working, and b) explain the coordinate system a little bit (see header for more info)
}

static bool key_pressed(int pKeyCode)
{
    return (GetAsyncKeyState(pKeyCode) & 0x8000) != 0;
}

bool rF2autocam::CheckHWControl( const char * const controlName, double &fRetVal )
{
  // only if enabled, of course
  if( !mEnabled )
    return( false );

  // Toggle auto camera: Ctrl + autokey (configured in ini, default Ctrl+F5).
  if (key_pressed(VK_CONTROL) && key_pressed(autokey))
  {
      if (!autokeypressed)
      {
          message.mDestination = 0;
          message.mTranslate = 0;
          if (automatic) {
              automatic = false;
              strcpy(message.mText, "Auto camera: off");
          }
          else {
              automatic = true;
              strcpy(message.mText, "Auto camera: on");
          }
      }
      autokeypressed = true;
  }
  else
  {
      // Reset when either key is released so next chord fires correctly
      autokeypressed = false;
  }
  // livecut (manual replay): poll the InstantReplay key ourselves so the operator
  // drives the replay. 1st press → enter replay (plugin jumps to the incident);
  // 2nd press → return to autocam. Frame-rate poll here (rF2); LMU polls in UpdateScoring.
  if (livecut && key_pressed(replaykey))
  {
      if (!replaykeypressed)
      {
          replaykeypressed = true;
          if (!onreplay) { onreplay = true; replaystarted = sessiontime; replayset = false; needreplay = false; }
          else           { onreplay = false; replayset = false; stopreplay = false; }
      }
  }
  else if (livecut)
  {
      replaykeypressed = false;
  }
  if ((_stricmp(controlName, "InstantReplay") == 0) && (sessiontime > (inctime + 10)) && (needreplay && !onreplay))
  {
	  fRetVal = 1.0f;
	  onreplay = true;
	  needreplay = false;
	  replaystarted = sessiontime;
	  replayset = false;	  
	  return (true);
  }
  if ((_stricmp(controlName, "InstantReplay") == 0) && (onreplay && stopreplay))
  {
	  stopreplay = false;
	  onreplay = false;
	  replayset = false;
	  needreplay = false;
	  fRetVal = 1.0f;
	  return (true);
  }
  return(false);
}


bool rF2autocam::ForceFeedback( double &forceValue )
{
  // Note that incoming value is the game's computation, in case you're interested.
#if 0 // enable to log it out (note that this is a very very slow implementation)
  FILE *fo = fopen( "FFB.txt", "a" );
  if( fo != NULL )
  {
    fprintf( fo, "\nFFB=%.4f", forceValue );
    fclose( fo );
  }
#endif

  // CHANGE COMMENTS TO ENABLE FORCE EXAMPLE
  return( false );

  // I think the bounds are -11500 to 11500 ...
//  forceValue = 11500.0 * sinf( mET );
//  return( true );
}


void rF2autocam::UpdateScoring(const ScoringInfoV01 &info)
{
    // Ctrl+autokey toggle — fallback for LMU (rF2 handles it via CheckHWControl with HUD)
    if (key_pressed(VK_CONTROL) && key_pressed(autokey)) {
        if (!autokeypressed) {
            automatic = !automatic;
            if (automatic) {
                // Skip the normal waitsec delay so the first switch fires immediately
                camvalttime = info.mCurrentET - waitsec - 5.0;
                strcpy(message.mText, "Auto camera: on");
            } else {
                strcpy(message.mText, "Auto camera: off");
            }
        }
        autokeypressed = true;
    } else {
        autokeypressed = false;
    }

    if (automatic == 0) return;

    // Note: called twice per second
    scoringrun = true;
    sessiontime = info.mCurrentET;

    // LMU: the native instant replay (R) can be ENTERED but not exited via R, and the plugin
    // can neither track nor control it. So we do NOT manage replay state on LMU — the sticky
    // live-focus below locks the camera on the incident car for incidenthold seconds (no
    // switching during that window = an effective stand-down). The operator presses native R
    // to replay the focused incident and native Esc to exit. Set incidenthold long enough to
    // cover watching the replay. (rF2 keeps full plugin-driven manual replay via CheckHWControl.)

    // Self-heal on session change / restart: rF2's StartSession callback is not always
    // delivered, so detect it here. Without this, stale camvalttime from a long previous
    // session stays "in the future" relative to the new session clock and the switch timer
    // never fires → camera freezes. (Observed: 50-min practice → qualifying.)
    //   - mSession change  → practice↔qual↔race transitions
    //   - camvalttime > sessiontime → clock jumped backwards (race restart, same mSession);
    //     camvalttime is otherwise always <= sessiontime, so this never false-fires.
    if (info.mSession != lastSession || camvalttime > sessiontime) {
        ResetSessionState();
        lastSession = info.mSession;
        sessiontime = info.mCurrentET; // ResetSessionState zeroed timing; keep current clock
    }

    finished    = 0;
    allfinished = true;
    camvalthat  = waitsec + (rand() % 5);

    if (info.mNumVehicles > 0) {
        ScanVehicles(info);

        if (info.mSession <= 4)      SelectCameraPractice(info);   // testday + practice 1-4
        else if (info.mSession < 10) SelectCameraQualifying(info); // qual 5-8 + warmup 9
        else                         SelectCameraRace(info);       // race 10-13

        if (allfinished) {
            camvalthat = waitsec + (rand() % 10);
            if ((aktpos < 3) && (walkthrough == 1)) {
                needpos          = aktpos + 1;
                pontosminbehind  = obtime + 1; // disable onboard
            } else {
                needpos          = 1;
                pontosminbehind  = obtime + 1;
            }
        }
        if (needpos == 0) { // nobody found (e.g. everyone in pit)
            camvalthat = 2;
            needpos    = 1;
        }

        DetectIncidents(info);
        // Clear the incident signal once the hold window elapses.
        if (incidentActive && (sessiontime > incidentSince + incidenthold))
            incidentActive = false;

        timerFired = ((sessiontime - camvalttime) > camvalthat);
        if (timerFired)
            ResolveTargetVehicle(info);
        else
            needveh = aktveh;

        // livecut: hold the live camera on the incident car for incidenthold seconds
        // so the operator can hit R to replay it. Overrides normal selection (rF2 + LMU).
        if (livecut && livecutFocus && !onreplay) {
            if (sessiontime > livecutFocusStart + incidenthold)
                livecutFocus = false;
            else {
                needveh = replayveh;
                needcam = kCamTrackside;
            }
        }
    }

    // Update driver name and current lap time for the tracked vehicle
    for (long i = 0; i < info.mNumVehicles; ++i)
    {
        VehicleScoringInfoV01 &vinfo = info.mVehicle[i];
        if (vinfo.mID == replayveh) replayname = vinfo.mDriverName;
        if (vinfo.mID == needveh)
        {
            aktname = vinfo.mDriverName;
            curlapT = info.mCurrentET - vinfo.mLapStartET;
        }
    }

    // LMU: WantsToViewVehicle is never called by LMU → switch camera via REST API.
    // Skip when player is driving their own car (avoid disrupting cockpit view).
    // Skip while onreplay (livecut): stand down so the native instant replay is not disturbed.
    if (isLMU && !playerDriving && !onreplay && needveh != aktveh) {
        aktveh      = needveh;
        aktpos      = needpos;
        lastcam     = needcam;
        camvalttime = sessiontime;
        WritetoFileDrivername();
        SwitchCameraViaREST(needveh);
    }

    WriteSessionOutputs(info);

    scoringrun = false;
}

// ── UpdateScoring sub-routines ────────────────────────────────────────────────

void rF2autocam::ScanVehicles(const ScoringInfoV01 &info)
{
    minbehind       = 99999;
    pontosminbehind = obtime + 1;
    numveh          = info.mNumVehicles;
    needpos = 0; needspos = 0; needdpos = 0; needsbspos = 0;
    inpit   = false;
    maxsbs  = 0;
    playerDriving = false;
    playerSlotId  = -1;
    dbgPlayerCtl  = -9;

    for (long i = 0; i < info.mNumVehicles; ++i)
    {
        VehicleScoringInfoV01 &vinfo = info.mVehicle[i];
        // Detect local player actively on track (not finished/DNF and not in pit area).
        // mPitState == 0 means on track; any other value means pit lane / pit box / garage.
        // This allows autocam to work while the player is waiting in the pits or watching
        // before going out, while still blocking camera switches during active laps.
        // "Player driving" = the local human is actively in control of a car (mControl==0),
        // not merely the owner of a slot. mIsPlayer is unreliable (esp. LMU: it stays set on
        // the owned car even while spectating). mControl tracks who is *actually* driving and
        // updates dynamically as the user jumps between driving and spectating mid-session.
        if (vinfo.mIsPlayer) dbgPlayerCtl = vinfo.mControl;     // diagnostic only
        if (vinfo.mControl == 0) {                              // 0 = local player in control
            playerSlotId = vinfo.mID;
            if (vinfo.mFinishStatus == 0 && vinfo.mPitState == 0)
                playerDriving = true;
        }
        if (vinfo.mPlace == 1)
        {
            first         = vinfo.mID;
            elso          = vinfo.mDriverName;
            completedlaps = vinfo.mTotalLaps;
        }
        if (vinfo.mFinishStatus == 0) { allfinished = false; }
        if ((vinfo.mBestLapTime < bestlapT) && (vinfo.mBestLapTime > 0))
        {
            bestlapT = vinfo.mBestLapTime;
            best1T   = vinfo.mBestSector1;
            best2T   = vinfo.mBestSector2;
        }
    }
}

// Pick a random on-track car that hasn't been featured yet this cycle (variety over
// strict order). Records the chosen car's mID; once everyone has had screen time the
// cycle resets. Returns the car's finishing position (mPlace), or 0 if nobody else is
// on track. Shared rotation base for practice and qualifying.
long rF2autocam::PickRandomUnshownCar(const ScoringInfoV01 &info)
{
    long candPos[64];
    long candId[64];
    int  n = 0;
    for (int pass = 0; pass < 2 && n == 0; ++pass)
    {
        if (pass == 1) shownCount = 0; // pass 0 found only already-shown cars → start a new cycle
        for (long i = 0; i < info.mNumVehicles && n < 64; ++i)
        {
            VehicleScoringInfoV01 &v = info.mVehicle[i];
            if (playerDriving && v.mIsPlayer) continue;             // skip player while on track
            if (v.mPitState != 0 || v.mFinishStatus != 0) continue; // on track only
            if (v.mID == aktveh) continue;                          // don't immediately re-pick current
            if (pass == 0)
            {
                bool shown = false;
                for (int k = 0; k < shownCount; ++k) if (shownCars[k] == v.mID) { shown = true; break; }
                if (shown) continue;
            }
            candPos[n] = v.mPlace;
            candId[n]  = v.mID;
            ++n;
        }
    }
    if (n == 0) return 0; // nobody else on track → caller holds current
    int idx = rand() % n;
    if (shownCount < 64) shownCars[shownCount++] = candId[idx];
    return candPos[idx];
}

// Practice: rotate through the cars circulating on track, one per dwell interval,
// random order with not-yet-shown cars preferred (no leader lock).
void rF2autocam::SelectCameraPractice(const ScoringInfoV01 &info)
{
    if ((sessiontime - camvalttime) >= camvalthat)
    {
        long p = PickRandomUnshownCar(info);
        if (p != 0) needpos = p;
    }
    else
    {
        needpos = aktpos; // hold current until the dwell elapses
    }
}

// Qualifying: same random rotation base, with priority overrides.
// Priority: lead change (new P1) > on pace for overall best > beating best S1 > rotation.
void rF2autocam::SelectCameraQualifying(const ScoringInfoV01 &info)
{
    for (long i = 0; i < info.mNumVehicles; ++i)
    {
        VehicleScoringInfoV01 &vinfo = info.mVehicle[i];
        if (playerDriving && vinfo.mIsPlayer) continue; // skip player while on track
        // Sector 2 completed and on pace for overall best
        if ((vinfo.mCurSector2 > 0) && ((vinfo.mCurSector2 + vinfo.mCurSector1) < (best1T + best2T)) && ((needdpos > vinfo.mPlace) || (needdpos == 0)))
        {
            if ((sessiontime - camvalttime) >= camvalthat) needdpos = vinfo.mPlace;
            pontosminbehind = 0.01; // allow onboard switch
        }
        // Sector 1 completed and faster than best S1
        if ((vinfo.mCurSector1 > 0) && (vinfo.mCurSector2 == 0) && (vinfo.mCurSector1 < best1T) && ((needspos > vinfo.mPlace) || (needspos == 0)))
        {
            if ((sessiontime - camvalttime) >= camvalthat) needspos = vinfo.mPlace;
            pontosminbehind = 0.01;
        }
    }

    // Detect a lead change: a different car now holds P1 (set a new overall best lap).
    bool leadChanged = false;
    if (lastLeader == -1)               lastLeader = first;            // prime at session start
    else if (first != lastLeader && first != 0) { leadChanged = true; lastLeader = first; }

    if (leadChanged)
    {
        needpos     = 1;                              // focus the new leader…
        camvalttime = sessiontime - camvalthat - 1.0; // …and cut to it immediately
    }
    else if (needdpos != 0)  needpos = needdpos;      // car on pace for overall best
    else if (needspos != 0)  needpos = needspos;      // car beating best S1
    else if ((sessiontime - camvalttime) >= camvalthat)
    {
        long p = PickRandomUnshownCar(info);          // nobody improving → rotate
        if (p != 0) needpos = p;
    }
    else needpos = aktpos;                            // hold current until the dwell elapses
}

void rF2autocam::SelectCameraRace(const ScoringInfoV01 &info)
{
    for (long i = 0; i < info.mNumVehicles; ++i)
    {
        VehicleScoringInfoV01 &vinfo = info.mVehicle[i];
        if ((vinfo.mFinishStatus == 1) && (finished < vinfo.mPlace)) { finished = vinfo.mPlace; }
        if (playerDriving && vinfo.mIsPlayer) continue; // skip player while on track
        if ((vinfo.mPitState == 0) && (vinfo.mFinishStatus == 0))
        {
            // Weight the gap: cars outside interest range get a penalty
            if ((vinfo.mPlace > interest) && (vinfo.mTimeBehindNext > obtime))
                aktbehind = round((abs(vinfo.mTimeBehindNext + 0.5) / 3) * 10);
            else
                aktbehind = round((abs(vinfo.mTimeBehindNext) / 3) * 10);

            // Count side-by-side cars
            sbs = 0;
            for (long j = 0; j < info.mNumVehicles; ++j)
            {
                VehicleScoringInfoV01 &vinfosbs = info.mVehicle[j];
                if (abs(vinfo.mLapDist - vinfosbs.mLapDist) < sbsdist) { sbs++; }
            }

            if ((info.mGamePhase == 5) || (info.mGamePhase == 4)) // Green Flag
            {
                if ((aktbehind <= minbehind) && (vinfo.mTimeBehindNext != 0))
                {
                    if (aktbehind == minbehind)
                    {
                        if ((vinfo.mPlace < needpos) || (needpos == 0))
                        {
                            minbehind       = aktbehind;
                            pontosminbehind = abs(vinfo.mTimeBehindNext);
                            needpos         = vinfo.mPlace;
                        }
                    }
                    else
                    {
                        minbehind       = aktbehind;
                        pontosminbehind = abs(vinfo.mTimeBehindNext);
                        needpos         = vinfo.mPlace;
                    }
                }
                if ((sbs > maxsbs) && (sbs >= sbscount) && ((needsbspos == 0) || (vinfo.mPlace < needsbspos)))
                {
                    maxsbs    = sbs;
                    needsbspos = vinfo.mPlace;
                }
            }
            else if (info.mGamePhase == 3) // Formation lap — cycle through field
            {
                if ((aktpos < info.mNumVehicles) && (walkthrough == 1)) needpos = aktpos + 1;
                else needpos = 1;
            }
            else // Safety car / other phases
            {
                camvalthat = 2;
                needpos    = 1;
            }
        }
    }

    // Side-by-side overrides normal selection
    if (maxsbs >= sbscount) {
        needpos         = needsbspos;
        pontosminbehind = obtime + 1; // disable onboard
    }

    // Show pit activity when the race action is calm enough
    inpit = false;
    if (((iequals(showinpit, "interestdiff")) && (pontosminbehind > interestsec))
        || ((iequals(showinpit, "onboarddiff")) && (pontosminbehind > obtime))
        || iequals(showinpit, "always"))
    {
        pitpos = 0;
        for (long i = 0; i < info.mNumVehicles; ++i)
        {
            VehicleScoringInfoV01 &vinfo = info.mVehicle[i];
            if (((vinfo.mPitState == 2) || (vinfo.mPitState == 4)) && (vinfo.mFinishStatus == 0) && ((pitpos == 0) || (pitpos > vinfo.mPlace))) {
                inpit  = true;
                pitpos = vinfo.mPlace;
            }
        }
    }
    else if (showinpitl > 0) // show top-N in pit
    {
        pitpos = 0;
        for (long i = 0; i < info.mNumVehicles; ++i)
        {
            VehicleScoringInfoV01 &vinfo = info.mVehicle[i];
            if ((vinfo.mPitState > 1) && (vinfo.mFinishStatus == 0) && ((pitpos == 0) || (pitpos > vinfo.mPlace)) && (vinfo.mPlace <= showinpitl)) {
                inpit  = true;
                pitpos = vinfo.mPlace;
            }
        }
    }
    if (inpit && (pitpos != 0)) {
        needpos         = pitpos;
        pontosminbehind = obtime + 1; // disable onboard
    }

    // Random camera switch when nobody is fighting (20% chance)
    if (pontosminbehind > interestsec) {
        if (((sessiontime - camvalttime) >= camvalthat) && (!inpit))
            if ((rand() % 5) == 1) needpos = rand() % info.mNumVehicles + 1;
    }

    // Focus on the last car not yet finished on their final lap
    for (long i = 0; i < info.mNumVehicles; ++i)
    {
        VehicleScoringInfoV01 &vinfo = info.mVehicle[i];
        if (vinfo.mFinishStatus == 0)
        {
            if ((((info.mMaxLaps - 1) == (vinfo.mTotalLaps)) || (((info.mEndET - sessiontime) < 20) && (info.mEndET > 0)))
                && (vinfo.mSector == 0) && (vinfo.mPlace == (finished + 1)))
            {
                needpos         = vinfo.mPlace;
                pontosminbehind = obtime + 1; // disable onboard
                camvalthat      = 2;
            }
            allfinished = false;
        }
    }
}

void rF2autocam::DetectIncidents(const ScoringInfoV01 &info)
{
    // Diff-based: process only newly appended text in mResultsStream.
    // On the first call after session start, establish a baseline to avoid
    // replaying stale incidents from the previous session.
    char* e = nullptr;
    sseged = "";
    {
        const std::string currentStream(info.mResultsStream);
        if (!prevResultsReady) {
            prevResultsStream = currentStream; // baseline only
            prevResultsReady  = true;
        } else if (currentStream.length() > prevResultsStream.length()) {
            sseged            = currentStream.substr(prevResultsStream.length());
            prevResultsStream = currentStream;
        } else if (currentStream.length() < prevResultsStream.length()) {
            prevResultsStream = currentStream; // unexpected reset
        }
    }

    std::size_t ifound  = sseged.find("Incident");
    std::size_t imfound = sseged.find("Immovable");
    std::size_t vfound  = sseged.find("vehicle");
    if ((ifound != std::string::npos) && ((imfound != std::string::npos) || (vfound != std::string::npos)))
    {
        sincsize[0] = '\0';
        std::size_t sbfound  = sseged.find("(");
        std::size_t sefound  = sseged.find(")");
        std::size_t sibfound = sseged.find("contact (");
        std::size_t siefound = sseged.find(") with");
        sseged.substr(sibfound + 9, siefound - sibfound - 9).copy(sincsize, 10);
        if ((strtod(sincsize, &e) > pincsize) && (strtod(sincsize, &e) > lowinc))
        {
            preplayveh = strtol(sseged.substr((sbfound + 1), sefound - (sbfound + 1)).c_str(), &e, 0);
            pincsize   = strtod(sincsize, &e);
            pinctime   = sessiontime;
        }
    }

    if ((!needreplay) && (!onreplay) && (sessiontime < (pinctime + 180)))
    {
        if ((pincsize >= highinc) || ((pincsize >= lowinc) && (info.mSession < 10)))
        {
            incidentActive = true;          // status.json "incident" + OBS signal
            incidentSince  = sessiontime;
            if (livecut) {
                // Sticky live-focus: lock onto the FIRST incident of a burst. A multi-car
                // pile-up emits several contact lines; ignoring later ones for the focus
                // window keeps the camera on the car we locked onto, so pressing R always
                // replays it (LMU cannot re-seek the native replay after the fact).
                if (!livecutFocus) {
                    incsize    = pincsize;
                    replayveh  = preplayveh;
                    inctime    = pinctime;
                    livecutFocus      = true;
                    livecutFocusStart = sessiontime;
                    strcpy(message.mText, "Incident - press R for replay");
                }
            } else {
                incsize    = pincsize;
                replayveh  = preplayveh;
                inctime    = pinctime;
                needreplay = true;          // auto instant replay (current behavior)
            }
            pincsize   = 0;
        }
        if ((pincsize >= lowinc) && (info.mSession < 10) && ((pontosminbehind <= obtime) && (pontosminbehind >= 0.04)))
        {
            incidentActive = true;          // status.json "incident" + OBS signal
            incidentSince  = sessiontime;
            if (livecut) {
                // Sticky live-focus: lock onto the FIRST incident of a burst. A multi-car
                // pile-up emits several contact lines; ignoring later ones for the focus
                // window keeps the camera on the car we locked onto, so pressing R always
                // replays it (LMU cannot re-seek the native replay after the fact).
                if (!livecutFocus) {
                    incsize    = pincsize;
                    replayveh  = preplayveh;
                    inctime    = pinctime;
                    livecutFocus      = true;
                    livecutFocusStart = sessiontime;
                    strcpy(message.mText, "Incident - press R for replay");
                }
            } else {
                incsize    = pincsize;
                replayveh  = preplayveh;
                inctime    = pinctime;
                needreplay = true;          // auto instant replay (current behavior)
            }
            pincsize   = 0;
        }
    }
}

void rF2autocam::ResolveTargetVehicle(const ScoringInfoV01 &info)
{
    for (long i = 0; i < info.mNumVehicles; ++i)
    {
        VehicleScoringInfoV01 &vinfo = info.mVehicle[i];
        if (vinfo.mPlace == needpos)
        {
            if (((vinfo.mPitState == 0) && (vinfo.mFinishStatus == 0)) || allfinished || inpit)
                needveh = vinfo.mID;
            else
                needveh = first;
        }
        if (needpos > 1)
        {
            if (vinfo.mPlace == needpos - 1)
                if (((vinfo.mPitState == 0) && (vinfo.mFinishStatus == 0)) || allfinished || inpit)
                    rvveh = vinfo.mID;
        }
        else { rvveh = first; }
    }

    if ((lastcam == 0) && (needveh == aktveh)) obchance = 3;
    else                                        obchance = 2;

    if ((pontosminbehind >= 0.05) && (pontosminbehind <= obtime) && ((rand() % 9 + 1) <= obchance))
    {
        if ((rand() % 99 + 1) <= rearview) {
            needcam = rvcam;
            needveh = rvveh;
        } else {
            needcam = obcam;
        }
    }
    else {
        needcam = kCamTrackside;
    }
}

void rF2autocam::WriteSessionOutputs(const ScoringInfoV01 &info)
{
    gamePhase = static_cast<long>(info.mGamePhase);

    // Build time display string (shared by time.txt and status.json)
    std::string timestr;
    if (onreplay)
    {
        timestr = "REPLAY";
    }
    else
    {
        char   tbuf[32] = {};
        double remain   = info.mEndET - info.mCurrentET;
        if (info.mSession > 9) {
            if ((info.mGamePhase > 4) && (info.mGamePhase < 8)) {
                currentlap = completedlaps + 1;
                if (completedlaps == 0) { completedlaps = 1; }
                if ((remain > 0) && ((((info.mCurrentET / completedlaps) * (info.mMaxLaps - 1)) > info.mEndET) || (info.mMaxLaps == 0)))
                {
                    snprintf(tbuf, sizeof(tbuf), "%02.0f:%02.0f:%02.0f", floor(remain / 3600.0), floor(fmod(remain, 3600.0) / 60.0), fmod(remain, 60.0));
                    timestr = tbuf;
                }
                else
                {
                    if ((currentlap == info.mMaxLaps) || ((info.mMaxLaps > kNoLapLimit) && (remain < 0)))
                        timestr = "Last lap";
                    else {
                        snprintf(tbuf, sizeof(tbuf), "%d / %d", currentlap, info.mMaxLaps);
                        timestr = tbuf;
                    }
                }
            }
            else { timestr = (info.mGamePhase == 8 ? "Race finished" : ""); }
        }
        else {
            if (remain > 0) {
                snprintf(tbuf, sizeof(tbuf), "%02.0f:%02.0f:%02.0f", floor(remain / 3600.0), floor(fmod(remain, 3600.0) / 60.0), fmod(remain, 60.0));
                timestr = tbuf;
            }
            else { timestr = "Session end"; }
        }
    }

    { std::ofstream f(timefname); if (f.is_open()) f << timestr; }
    WritetoInfohtml(info.mSession);
    WriteToJson(info.mSession, timestr);

    if (debuglog) {
        std::ofstream fo(filespath + "\\debug.log", std::ios::app);
        if (fo.is_open()) {
            fo << std::fixed << std::setprecision(3)
               << "t="          << sessiontime
               << " ses="       << info.mSession
               << " ph="        << (int)info.mGamePhase
               << " sip="       << showinpit
               << " intdiff="   << interestsec
               << " obdiff="    << obtime
               << " wait="      << waitsec
               << " pmb="       << pontosminbehind
               << " mb="        << minbehind
               << " npos="      << needpos
               << " apos="      << aktpos
               << " nveh="      << needveh
               << " aveh="      << aktveh
               << " rveh="      << replayveh
               << " imag="      << incsize
               << " lo="        << lowinc
               << " hi="        << highinc
               << " inc="       << (incidentActive ? 1 : 0)
               << " lcf="       << (livecutFocus ? 1 : 0)
               << " orp="       << (onreplay ? 1 : 0)
               << " lc="        << livecut
               << " pctl="      << (int)dbgPlayerCtl
               << " inpit="     << inpit
               << " sbs="       << maxsbs
               << " pdrv="      << (playerDriving ? 1 : 0)
               << " psid="      << playerSlotId
               << " tfired="    << (timerFired ? 1 : 0)
               << " wtvN="      << refreshcount
               << " rep="       << dbgReplayActive
               << " wpath="     << dbgWtvPath
               << " stream_len=" << strlen(info.mResultsStream)
               << "\n";
        refreshcount = 0;
        }
    }
}

bool rF2autocam::RequestCommentary( CommentaryRequestInfoV01 &info )
{
  // COMMENT OUT TO ENABLE EXAMPLE
  return( false );

  // only if enabled, of course
  if( !mEnabled )
    return( false );

  // Note: function is called twice per second

  // Say green flag event for no particular reason every 20 seconds ...
  const double timeMod20 = fmod( mET, 20.0 );
  if( timeMod20 > 19.0 )
  {
    strcpy( info.mName, "GreenFlag" );
    info.mInput1 = 0.0;
    info.mInput2 = 0.0;
    info.mInput3 = 0.0;
    info.mSkipChecks = true;
    return( true );
  }

  return( false );
}

unsigned char rF2autocam::WantsToViewVehicle(CameraControlInfoV01 &camControl)
{
	++refreshcount; // how many times rF2 calls this per UpdateScoring interval
	dbgReplayActive = camControl.mReplayActive ? 1 : 0;
	// dbgWtvPath codes: 0=gate failed (scoringrun/!automatic), 1=replay branch handled,
	//   2=committed (return 1), 3=player-guard return 0, 4=replay-active no-op (commit skipped),
	//   5=fell through (needveh==aktveh && needcam==lastcam)
	dbgWtvPath = 0;
	if ((!scoringrun) && (automatic != 0)) {
		// Gate on our own incident replay (onreplay), NOT rF2's mReplayActive.
		// rF2 latches mReplayActive=true after the player drives and never clears it,
		// which previously froze the normal commit block below. (Confirmed via rep/wpath diag.)
		if (onreplay)
		{
			dbgWtvPath = 4; // our incident replay active: normal commit block below is skipped
			// livecut: the operator ends the replay with a 2nd R press, so skip the auto-stop.
			if (!livecut && sessiontime > replaystarted + replayduration)
			{
				stopreplay = true;
				replayset = false;
				needreplay = false;
			}
			if (!replayset && !stopreplay && onreplay)
			{
				camControl.mID = replayveh;
				camControl.mCameraType = 4;
				camControl.mReplaySetTime = true;
				camControl.mReplaySeconds = static_cast<float>(inctime - replayoffset);
				aktveh = replayveh;				
				replayset = true;
				WritetoFileDrivername();
				return{ 3 };
			}
			if ((sessiontime > replaystarted + 4) && (sessiontime < replaystarted + 13))
			{
				camControl.mReplayCommand = 8;
				return{ 2 };
			}
			if (sessiontime > replaystarted + 13)
			{
				camControl.mReplayCommand = 9;
				return{ 2 };
			}						
		}
		if (!onreplay)
		{
			// Timer fired: force re-commit so autocam overrides any manual camera change.
			if (timerFired) {
				timerFired = false;
				aktveh = -1;
			}
			// Never commit to player's own car while they are actively on track.
			if (playerDriving && playerSlotId != -1 && needveh == playerSlotId)
				{ dbgWtvPath = 3; return{ 0 }; }
			if (needveh != aktveh)
			{
				dbgWtvPath = 2;
				camControl.mID = needveh;
				if (iequals(camtest, "ob")) { needcam = obcam; }
				else if (iequals(camtest, "rv")) { needcam = rvcam; }
				camControl.mCameraType = needcam;
				camvalttime = sessiontime;
				aktveh = needveh;
				aktpos = needpos;
				lastcam = needcam;
				WritetoFileDrivername();
				return{ 1 };
			}
			if (needcam != lastcam)
			{
				dbgWtvPath = 2;
				camControl.mID = needveh;
				if (iequals(camtest, "ob")) { needcam = obcam; }
				else if (iequals(camtest, "rv")) { needcam = rvcam; }
				camControl.mCameraType = needcam;
				camvalttime = sessiontime;
				aktveh = needveh;
				aktpos = needpos;
				lastcam = needcam;
				WritetoFileDrivername();
				return{ 1 };
			}
			dbgWtvPath = 5; // non-replay, already on target (needveh==aktveh, needcam==lastcam)
		}
	}
	return{ 0 };

}

bool rF2autocam::WantsToDisplayMessage(MessageInfoV01 &msgInfo)
{
	if (message.mText[0] == '\0')
	{
		return (false);
	}
	else {
		msgInfo = message;
		msgInfo.mDestination = isLMU ? 1 : 0;  // LMU: chat (0=message centre not supported)
		strcpy(message.mText, "");
		return (true);
	}
}
