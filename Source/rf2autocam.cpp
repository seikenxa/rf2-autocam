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

// plugin information

extern "C" __declspec( dllexport )
const char * __cdecl GetPluginName()                   { return( "rF2 autocam - 2026.05.24." ); }

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
    aktpos        = 0;
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

	f << "{\n";
	if (onreplay) {
		f << "  \"driver\": \""   << jsonEscape(replayname) << "\",\n";
		f << "  \"position\": 0,\n";
	} else {
		f << "  \"driver\": \""   << jsonEscape(aktname) << "\",\n";
		f << "  \"position\": "   << aktpos << ",\n";
	}
	f << "  \"camera\": \""       << camname  << "\",\n";
	f << "  \"on_replay\": "      << (onreplay ? "true" : "false") << ",\n";
	f << "  \"autocam\": "        << (automatic ? "true" : "false") << ",\n";
	f << "  \"session_type\": \"" << sessname << "\",\n";
	f << "  \"time_display\": \"" << jsonEscape(timestr) << "\",\n";
	f << "  \"gap_to_next\": "
	  << std::fixed << std::setprecision(3) << pontosminbehind << "\n";
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
		lowinc = 0.4;
		WritePrivateProfileString("AUTOCAM", "lowinc", "0.4", str.c_str());
	}
	GetPrivateProfileString("AUTOCAM", "highinc", "a", seged, sizeof(seged), str.c_str());
	highinc = strtod(seged, &e);
	if (0 == highinc && seged == e) {
		highinc = 0.8;
		WritePrivateProfileString("AUTOCAM", "highinc", "0.8", str.c_str());
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
        strcpy(message.mText, "rF2autocam 2026.05.24. Auto camera: on");
    } else {
        strcpy(message.mText, "rF2autocam 2026.05.24. Auto camera: off");
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
        if (!autokeypressed) { automatic = !automatic; }
        autokeypressed = true;
    } else {
        autokeypressed = false;
    }

    if (automatic == 0) return;

    // Note: called twice per second
    scoringrun = true;
    sessiontime = info.mCurrentET;
    finished    = 0;
    allfinished = true;
    camvalthat  = waitsec + (rand() % 5);

    if (info.mNumVehicles > 0) {
        ScanVehicles(info);

        if (info.mSession < 10) SelectCameraQualifying(info);
        if (info.mSession > 9)  SelectCameraRace(info);

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

        if ((sessiontime - camvalttime) > camvalthat)
            ResolveTargetVehicle(info);
        else
            needveh = aktveh;
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

    // LMU: WantsToViewVehicle is never called by LMU → switch camera via REST API
    if (isLMU && needveh != aktveh) {
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

    for (long i = 0; i < info.mNumVehicles; ++i)
    {
        VehicleScoringInfoV01 &vinfo = info.mVehicle[i];
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

void rF2autocam::SelectCameraQualifying(const ScoringInfoV01 &info)
{
    for (long i = 0; i < info.mNumVehicles; ++i)
    {
        VehicleScoringInfoV01 &vinfo = info.mVehicle[i];
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
        // Any car on a flying lap
        if ((vinfo.mCurSector1 > 0) && ((needpos > vinfo.mPlace) || (needpos == 0)))
        {
            if ((sessiontime - camvalttime) >= camvalthat) needpos = vinfo.mPlace;
            pontosminbehind = 0.01;
        }
    }
    // Promote to best candidate found
    if (needspos != 0) needpos = needspos;
    if (needdpos != 0) needpos = needdpos;
    // Fallback: pick any car on track
    if (needpos == 0) {
        for (long i = 0; i < info.mNumVehicles; ++i)
        {
            VehicleScoringInfoV01 &vinfo = info.mVehicle[i];
            if (((vinfo.mPitState == 0) || (vinfo.mPitState == 4)) && ((needpos > vinfo.mPlace) || (needpos == 0)))
                needpos = vinfo.mPlace;
        }
    }
}

void rF2autocam::SelectCameraRace(const ScoringInfoV01 &info)
{
    for (long i = 0; i < info.mNumVehicles; ++i)
    {
        VehicleScoringInfoV01 &vinfo = info.mVehicle[i];
        if ((vinfo.mFinishStatus == 1) && (finished < vinfo.mPlace)) { finished = vinfo.mPlace; }
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
            incsize    = pincsize;
            replayveh  = preplayveh;
            inctime    = pinctime;
            needreplay = true;
            pincsize   = 0;
        }
        if ((pincsize >= lowinc) && (info.mSession < 10) && ((pontosminbehind <= obtime) && (pontosminbehind >= 0.04)))
        {
            incsize    = pincsize;
            replayveh  = preplayveh;
            inctime    = pinctime;
            needreplay = true;
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
               << " inpit="     << inpit
               << " sbs="       << maxsbs
               << " stream_len=" << strlen(info.mResultsStream)
               << "\n";
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
	if ((!scoringrun) && (automatic != 0)) {
		if (camControl.mReplayActive)
		{
			if (sessiontime > replaystarted + replayduration)
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
		if (!camControl.mReplayActive)
		{
			if (needveh != aktveh)
			{
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
		}
	}
	return{ 0 };

}

bool rF2autocam::WantsToDisplayMessage(MessageInfoV01 &msgInfo)
{
	if (message.mText == "")
	{
		return (false);
	}
	else {
		msgInfo = message;
		strcpy(message.mText, "");
		return (true);
	}
}
