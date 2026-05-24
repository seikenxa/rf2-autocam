//ÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜ
//Ý                                                                         Þ
//Ý Module: Internals Example Header File                                   Þ
//Ý                                                                         Þ
//Ý Description: Declarations for the Internals Example Plugin              Þ
//Ý                                                                         Þ
//Ý                                                                         Þ
//Ý This source code module, and all information, data, and algorithms      Þ
//Ý associated with it, are part of CUBE technology (tm).                   Þ
//Ý                 PROPRIETARY AND CONFIDENTIAL                            Þ
//Ý Copyright (c) 1996-2014 Image Space Incorporated.  All rights reserved. Þ
//Ý                                                                         Þ
//Ý                                                                         Þ
//Ý Change history:                                                         Þ
//Ý   tag.2005.11.30: created                                               Þ
//Ý                                                                         Þ
//ßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßß

#ifndef _INTERNALS_EXAMPLE_H
#define _INTERNALS_EXAMPLE_H

#include "InternalsPlugin.hpp"
#include <string>


// This is used for the app to use the plugin for its intended purpose
class rF2autocam : public InternalsPluginV07  // REMINDER: exported function GetPluginVersion() should return 1 if you are deriving from this InternalsPluginV01, 2 for InternalsPluginV02, etc.
{

 public:
	bool environmentAlreadySet = false;

  // Constructor/destructor
	rF2autocam() = default;
	~rF2autocam() = default;

  // These are the functions derived from base class InternalsPlugin
  // that can be implemented.
  void Startup( long version );  // game startup
  void Shutdown();               // game shutdown

  void EnterRealtime();          // entering realtime
  void ExitRealtime();           // exiting realtime

  void StartSession();           // session has started
  void EndSession();             // session has ended

  // GAME OUTPUT
  long WantsTelemetryUpdates() { return( 0 ); } // CHANGE TO 1 TO ENABLE TELEMETRY EXAMPLE!
  void UpdateTelemetry( const TelemInfoV01 &info );

  bool WantsGraphicsUpdates() { return( false ); } // CHANGE TO TRUE TO ENABLE GRAPHICS EXAMPLE!
  void UpdateGraphics( const GraphicsInfoV01 &info );

  // GAME INPUT
  bool HasHardwareInputs() { return( true ); } // CHANGE TO TRUE TO ENABLE HARDWARE EXAMPLE!
  void UpdateHardware( const double fDT ) { mET += fDT; } // update the hardware with the time between frames
  void EnableHardware() { mEnabled = true; }             // message from game to enable hardware
  void DisableHardware() { mEnabled = false; }           // message from game to disable hardware

  // See if the plugin wants to take over a hardware control.  If the plugin takes over the
  // control, this method returns true and sets the value of the double pointed to by the
  // second arg.  Otherwise, it returns false and leaves the double unmodified.
  bool CheckHWControl( const char * const controlName, double &fRetVal );

  bool ForceFeedback( double &forceValue );  // SEE FUNCTION BODY TO ENABLE FORCE EXAMPLE

  // SCORING OUTPUT
  bool WantsScoringUpdates() { return( true ); } // CHANGE TO TRUE TO ENABLE SCORING EXAMPLE!
  void UpdateScoring( const ScoringInfoV01 &info );

  // COMMENTARY INPUT
  bool RequestCommentary( CommentaryRequestInfoV01 &info );  // SEE FUNCTION BODY TO ENABLE COMMENTARY EXAMPLE

  // VIDEO EXPORT (sorry, no example code at this time)
  virtual bool WantsVideoOutput() { return(false); }         // whether we want to export video
  virtual bool VideoOpen(const char * const szFilename, float fQuality, unsigned short usFPS, unsigned long fBPS,
	  unsigned short usWidth, unsigned short usHeight, char *cpCodec = NULL) {
	  return(false);
  } // open video output file
  virtual void VideoClose() {}                                 // close video output file
  virtual void VideoWriteAudio(const short *pAudio, unsigned int uNumFrames) {} // write some audio info
  virtual void VideoWriteImage(const unsigned char *pImage) {} // write video image 

  void SetEnvironment(const EnvironmentInfoV01 &info);  

 private:
  // ISI plugin base
  double mET      = 0.0;
  bool   mEnabled = false;

  // INI configuration
  long        waitsec     = 0;
  long        interest    = 0;
  double      obtime      = 0.0;
  long        interestsec = 0;
  long        automatic   = 0;
  long        walkthrough = 0;
  std::string showinpit;
  long        showinpitl  = 0;
  std::string camtest;
  long        obcam       = 0;
  long        rvcam       = 0;
  long        rearview    = 0;
  std::string inifilename;
  std::string filespath;
  std::string sfilename;
  double      lowinc  = 0.0;
  double      highinc = 0.0;
  long        debuglog = 0;    // debug log to file (0=off, 1=on)
  double      sbsdist  = 1.5;  // side-by-side detection distance [m]
  long        sbscount = 2;    // min cars at same point to trigger SBS camera
  long        replayduration = 20;   // instant replay playback length [s]
  double      replayoffset   = 5.0;  // seek this many seconds before incident [s]

  // Runtime camera state
  std::string    sseged;
  MessageInfoV01 message      = {};
  long           refreshcount = 0;
  long           camvalthat   = 0;
  bool           scoringrun   = false;
  long           needpos      = 0;
  long           needspos     = 0;
  long           needdpos     = 0;
  long           pitpos       = 0;
  long           needveh      = 0;
  long           rvveh        = 0;
  long           first        = 0;
  long           needcam      = 4;
  long           lastcam      = 0;
  long           aktveh       = -1;
  long           aktpos       = 0;
  long           sbs          = 0;
  long           needsbspos   = 0;
  long           maxsbs       = 0;
  double         minbehind         = 99999.0;
  double         pontosminbehind   = 0.0;
  double         aktbehind         = 0.0;
  double         sessiontime       = 0.0;
  double         camvalttime       = 0.0;
  bool           allfinished = false;
  long           finished    = 0;
  short          obchance    = 0;
  bool           inpit       = false;

  // Lap timing
  double bestlapT = 2147483640.0;
  double best1T   = 2147483640.0;
  double best2T   = 2147483640.0;
  double curlapT  = 0.0;

  // Toggle state for "Control - Custom Plugin #1"
  bool autokeypressed = false;

  // Incident replay
  bool   needreplay    = false;
  bool   stopreplay    = false;
  bool   onreplay      = false;
  double inctime       = 0.0;
  double incsize       = 0.0;
  long   replayveh     = -1;
  double pinctime      = 0.0;
  double pincsize      = 0.0;
  long   preplayveh    = -1;
  double replaystarted = 0.0;
  bool   replayset     = false;
  char   sincsize[10]  = {};

  // File output
  std::string driverfname;
  std::string timefname;
  std::string listfname;
  std::string jsonfname;
  std::string aktname;
  std::string replayname;
  std::string elso;
  long completedlaps  = 0;
  long currentlap     = 0;
  long numveh         = 0;

  virtual unsigned char WantsToViewVehicle(CameraControlInfoV01 &camControl);
  virtual bool WantsToDisplayMessage(MessageInfoV01 &msgInfo);
  void WritetoFileDrivername();
  void WritetoInfohtml(long session);
  void WriteToJson(long session, const std::string& timestr);
};


#endif // _INTERNALS_EXAMPLE_H

