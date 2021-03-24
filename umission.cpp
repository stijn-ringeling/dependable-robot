/***************************************************************************
 *   Copyright (C) 2016-2020 by DTU (Christian Andersen)                        *
 *   jca@elektro.dtu.dk                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Lesser General Public License for more details.                   *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/



#include <sys/time.h>
#include <cstdlib>

#include "umission.h"
#include "utime.h"
#include "ulibpose2pose.h"

#define SEESAW_TILT 150

UMission::UMission(UBridge * regbot, UCamera * camera)
{
  cam = camera;
  bridge = regbot;
  threadActive = 100;
  // initialize line list to empty
  for (int i = 0; i < missionLineMax; i++)
  { // add to line list 
    lines[i] = lineBuffer[i];  
    formatOutput[i] = formatOutputBuffer[i];
    // terminate c-strings strings - good practice, but not needed
    lines[i][0] = '\0';
    formatOutput[i][0] = '\0';
  }
  //Create loader
  loader = new ULoader("./missions");
  // start mission thread
  th1 = new thread(runObj, this);
//   play.say("What a nice day for a stroll\n", 100);
//   sleep(5);
}


UMission::~UMission()
{
  printf("Mission class destructor\n");
}


void UMission::run()
{
  while (not active and not th1stop)
    usleep(100000);
//   printf("UMission::run:  active=%d, th1stop=%d\n", active, th1stop);
  if (not th1stop)
    runMission();
  printf("UMission::run: mission thread ended\n");
}
  
void UMission::printStatus()
{
  printf("# ------- Mission ----------\n");
  printf("# active = %d, finished = %d\n", active, finished);
  printf("# mission part=%d, in state=%d\n", mission, missionState);
}
  
/**
 * Initializes the communication with the robobot_bridge and the REGBOT.
 * It further initializes a (maximum) number of mission lines 
 * in the REGBOT microprocessor. */
void UMission::missionInit()
{ // stop any not-finished mission
  bridge->send("robot stop\n");
  // clear old mission
  bridge->send("robot <clear\n");
  //
  // add new mission with 3 threads
  // one (100) starting at event 30 and stopping at event 31
  // one (101) starting at event 31 and stopping at event 30
  // one (  1) used for idle and initialisation of hardware
  // the mission is started, but staying in place (velocity=0, so servo action)
  //
  bridge->send("robot <add thread=1\n");
  // Irsensor should be activated a good time before use 
  // otherwise first samples will produce "false" positive (too short/negative).
  bridge->send("robot <add irsensor=1,vel=0:dist<0.2\n");
  //
  // alternating threads (100 and 101, alternating on event 30 and 31 (last 2 events)
  bridge->send("robot <add thread=100,event=30 : event=31\n");
  for (int i = 0; i < missionLineMax; i++)
    // send placeholder lines, that will never finish
    // are to be replaced with real mission
    // NB - hereafter no lines can be added to these threads, just modified
    bridge->send("robot <add vel=0 : time=0.1\n");
  //
  bridge->send("robot <add thread=101,event=31 : event=30\n");
  for (int i = 0; i < missionLineMax; i++)
    // send placeholder lines, that will never finish
    bridge->send("robot <add vel=0 : time=0.1\n");
  usleep(10000);
  //
  //
  // send subscribe to bridge
  bridge->pose->subscribe();
  bridge->edge->subscribe();
  bridge->motor->subscribe();
  bridge->event->subscribe();
  bridge->joy->subscribe();
  bridge->motor->subscribe();
  bridge->info->subscribe();
  bridge->irdist->subscribe();
  bridge->imu->subscribe();
  usleep(10000);
  // there maybe leftover events from last mission
  bridge->event->clearEvents();
}


void UMission::sendAndActivateSnippet(char ** missionLines, int missionLineCnt)
{
  // Calling sendAndActivateSnippet automatically toggles between thread 100 and 101. 
  // Modifies the currently inactive thread and then makes it active. 
  const int MSL = 100;
  char s[MSL];
  int threadToMod = 101;
  int startEvent = 31;
  // select Regbot thread to modify
  // and event to activate it
  if (threadActive == 101)
  {
    threadToMod = 100;
    startEvent = 30;
  }
  if (missionLineCnt > missionLineMax)
  {
    printf("# ----------- error - too many lines ------------\n");
    printf("# You tried to send %d lines, but there is buffer space for %d only!\n", missionLineCnt, missionLineMax);
    printf("# set 'missionLineMax' to a higher number in 'umission.h' about line 57\n");
    printf("# (not all lines will be send)\n");
    printf("# -----------------------------------------------\n");
    missionLineCnt = missionLineMax;
  }
  // send mission lines using '<mod ...' command
  for (int i = 0; i < missionLineCnt; i++)
  { // send lines one at a time
    if (strlen((char*)missionLines[i]) > 0)
    { // send a modify line command
        printf("Sending mission line: %s\n", missionLines[i]);
      snprintf(s, MSL, "<mod %d %d %s\n", threadToMod, i+1, missionLines[i]);
      bridge->send(s); 
    }
    else
      // an empty line will end code snippet too
      break;
  }
  // let it sink in (10ms)
  usleep(10000);
  // Activate new snippet thread and stop the other  
  snprintf(s, MSL, "<event=%d\n", startEvent);
  bridge->send(s);
  // save active thread number
  threadActive = threadToMod;
}


//////////////////////////////////////////////////////////

/**
 * Thread for running the mission(s)
 * All missions segments are called in turn based on mission number
 * Mission number can be set at parameter when starting mission command line.
 * 
 * The loop also handles manual override for the gamepad, and resumes
 * when manual control is released.
 * */
void UMission::runMission()
{ /// current mission number
  mission = fromMission;
  //mission = 4;
  int missionOld = mission;
  bool regbotStarted = false;
  /// end flag for current mission
  bool ended = false;
  /// manuel override - using gamepad
  bool inManual = false;
  /// debug loop counter
  int loop = 0;
  // keeps track of mission state
  missionState = 0;
  int missionStateOld = missionState;
  // fixed string buffer
  const int MSL = 120;
  char s[MSL];
  /// initialize robot mission to do nothing (wait for mission lines)
  missionInit();
  /// start (the empty) mission, ready for mission snippets.
  bridge->send("start\n"); // ask REGBOT to start controlled run (ready to execute)
  bridge->send("oled 3 waiting for REGBOT\n");
//   play.say("Waiting for robot data.", 100);
  ///
  for (int i = 0; i < 3; i++)
  {
    if (not bridge->info->isHeartbeatOK())
    { // heartbeat should come at least once a second
      sleep(2);
    }
  }
  if (not bridge->info->isHeartbeatOK())
  { // heartbeat should come at least once a second
    play.say("Oops, no usable connection with robot.", 100);
//    system("espeak \"Oops, no usable connection with robot.\" -ven+f4 -s130 -a60 2>/dev/null &"); 
    bridge->send("oled 3 Oops: Lost REGBOT!");
    printf("# ---------- error ------------\n");
    printf("# No heartbeat from robot. Bridge or REGBOT is stuck\n");
//     printf("# You could try restart ROBOBOT bridge ('b' from mission console) \n");
    printf("# -----------------------------\n");
    stop();
  }
  /// loop in sequence every mission until they report ended
  while (not finished and not th1stop)
  { // stay in this mission loop until finished
    loop++;
    // test for manuel override (joy is short for joystick or gamepad)
    if (bridge->joy->manual)
    { // just wait, do not continue mission
      usleep(20000);
      if (not inManual)
      {
//         system("espeak \"Mission paused.\" -ven+f4 -s130 -a40 2>/dev/null &"); 
        play.say("Mission paused.", 90);
      }
      inManual = true;
      bridge->send("oled 3 GAMEPAD control\n");
    }
    else
    { // in auto mode
      if (not regbotStarted)
      { // wait for start event is received from REGBOT
        // - in response to 'bot->send("start\n")' earlier
        if (bridge->event->isEventSet(33))
        { // start mission (button pressed)
//           printf("Mission::runMission: starting mission (part from %d to %d)\n", fromMission, toMission);
          regbotStarted = true;
        }
      }
      else
      { // mission in auto mode
        if (inManual)
        { // just entered auto mode, so tell.
          inManual = false;
//           system("espeak \"Mission resuming.\" -ven+f4 -s130 -a40 2>/dev/null &");
          play.say("Mission resuming", 90);
          bridge->send("oled 3 running AUTO\n");
        }
        switch(mission)
        {
          case 1: // running auto mission
            ended = mission1(missionState);
            break;
          case 2:
            ended = mission2(missionState);
            break;
          case 3:
            ended = mission3(missionState);
            break;
          case 4:
            ended = mission4(missionState);
            break;
          default:
            // no more missions - end everything
            finished = true;
            break;
        }
        if (ended)
        { // start next mission part in state 0
          mission++;
          ended = false;
          missionState = 0;
        }
        // show current state on robot display
        if (mission != missionOld or missionState != missionStateOld)
        { // update small O-led display on robot - when there is a change
          UTime t;
          t.now();
          snprintf(s, MSL, "oled 4 mission %d state %d\n", mission, missionState);
          bridge->send(s);
          if (logMission != NULL)
          {
            fprintf(logMission, "%ld.%03ld %d %d\n", 
                    t.getSec(), t.getMilisec(),
                    missionOld, missionStateOld
            );
            fprintf(logMission, "%ld.%03ld %d %d\n", 
                    t.getSec(), t.getMilisec(),
                    mission, missionState
            );
          }
          missionOld = mission;
          missionStateOld = missionState;
        }
      }
    }
    //
    // check for general events in all modes
    // gamepad buttons 0=green, 1=red, 2=blue, 3=yellow, 4=LB, 5=RB, 6=back, 7=start, 8=Logitech, 9=A1, 10 = A2
    // gamepad axes    0=left-LR, 1=left-UD, 2=LT, 3=right-LR, 4=right-UD, 5=RT, 6=+LR, 7=+-UD
    // see also "ujoy.h"
    if (bridge->joy->button[BUTTON_RED])
    { // red button -> save image
      if (not cam->saveImage)
      {
        printf("UMission::runMission:: button 1 (red) pressed -> save image\n");
        cam->saveImage = true;
      }
    }
    if (bridge->joy->button[BUTTON_YELLOW])
    { // yellow button -> make ArUco analysis
      if (not cam->doArUcoAnalysis)
      {
        printf("UMission::runMission:: button 3 (yellow) pressed -> do ArUco\n");
        cam->doArUcoAnalysis = true;
      }
    }
    // are we finished - event 0 disables motors (e.g. green button)
    if (bridge->event->isEventSet(0))
    { // robot say stop
      finished = true;
      printf("Mission:: insist we are finished\n");
    }
    else if (mission > toMission)
    { // stop robot
      // make an event 0
      bridge->send("stop\n");
      // stop mission loop
      finished = true;
    }
    // release CPU a bit (10ms)
    usleep(10000);
  }
  bridge->send("stop\n");
  snprintf(s, MSL, "Robot %s finished.\n", bridge->info->robotname);
//   system(s); 
  play.say(s, 100);
  printf("%s", s);
  bridge->send("oled 3 finished\n");
}


////////////////////////////////////////////////////////////

/**
 * Run mission
 * \param state is kept by caller, but is changed here
 *              therefore defined as reference with the '&'.
 *              State will be 0 at first call.
 * \returns true, when finished. */
bool UMission::mission1(int & state)
{
  bool finished = false;
  int linecount = 0;
  float tilt = 0.0;
  // First commands to send to robobot in given mission
  // (robot sends event 1 after driving 1 meter)):
  switch (state)
  {
    case 0:
      // tell the operatior what to do
      printf("# press green to start.\n");
//       system("espeak \"press green to start\" -ven+f4 -s130 -a5 2>/dev/null &");
      play.say("Press green to start", 90);
      bridge->send("oled 5 press green to start");
      state++;
      break;
    case 1:
      if (bridge->joy->button[BUTTON_GREEN]){
        //bridge->event->eventFlags[5] = true;
        //state = 23;
        state = 10;
      }
      break;
    case 10: // first PART - wait for IR2 then go fwd and turn
      
      loader->loadMission("01_follow_line_till_seesaw.mission", lines, &linecount);
      sendAndActivateSnippet(lines, linecount);
      //clear event 1
      bridge->event->isEventSet(1);
      // tell the operator
      printf("# case=%d sent mission snippet 1 -> go to the seesaw\n", state);
      //system("espeak \"code snippet 1.\" -ven+f4 -s130 -a5 2>/dev/null &"); 
      play.say("Code snippet 1.", 90);
      bridge->send("oled 5 code snippet 1");
      //
      // play as we go
      play.setFile("../The_thing_goes_Bassim.mp3");
      play.setVolume(100); // % (0..100)
      play.start();
      // go to wait for finished
      state = 11;
      break;
    case 11:
      // wait until the robot reach the crossing line -> event=1 set
      if (bridge->event->isEventSet(1))
      { // finished first drive
        bridge->event->isEventSet(2);
        state++;
        printf("# case=%d event 1 sensed -> robot at the crossing line\n", state);
        play.stopPlaying();
      }
      break;
    case 12:
      // wait for event 2 -> catch the ball
      // bridge->imu->gyro[0]  Rotation velocity around x-axis [degree/sec]
      // bridge->imu->gyro[1]  y-axis
      // bridge->imu->gyro[2]  z-axis
      if(bridge->event->isEventSet(2)){ //condition of stop
        // load stop mission and send it to the RoboBot
        printf("# case=%d event 2 sensed - stop and catch the ball\n",state);
        loader->loadMission("02_catch_ball.mission", lines, &linecount);
        bridge->event->isEventSet(3);
        sendAndActivateSnippet(lines, linecount);

        state = 21;
      } 
      break;
    case 21:
      if (bridge->event->isEventSet(3)){
        loader->loadMission("03_go_off_seesaw.mission", lines, &linecount);
        bridge->event->isEventSet(4);
        sendAndActivateSnippet(lines, linecount);
        printf("# case=%d event 3 sensed -> roobt go off the seesaw turning toward the direction to the ramp");
        state++;
      }
      break;
    default:
      printf("mission 1 ended \n");
      bridge->send("oled 5 \"mission 1 ended.\"");
      finished = true;
      break;
  }
  return finished;
}


/**
 * Run mission1: follow line sinse the crossing line, follow left line until the seesaw tilt
 * \param state is kept by caller, but is changed here
 *              therefore defined as reference with the '&'.
 *              State will be 0 at first call.
 * \returns true, when finished. */
bool UMission::mission2(int & state)
{
  bool finished = false;
  int linecount = 0;
  //Skip second mission
  //return true;
  // First commands to send to robobot in given mission
  // (robot sends event 1 after driving 1 meter)):
  switch (state)
  {
    case 0:
      // tell the operatior what to do
      printf("# started mission 2.\n");
//       system("espeak \"looking for ArUco\" -ven+f4 -s130 -a5 2>/dev/null &"); 
      play.say("Mission 2 - bring ball in the hole", 90);
      bridge->send("oled 5 mission 2 started");
      state=11;
      break;
    case 11:
      if (bridge->event->isEventSet(4)){
        loader->loadMission("01_drive_until_ramp.mission", lines, &linecount);
        bridge->event->isEventSet(5);
        sendAndActivateSnippet(lines, linecount);
        printf("# case=%d event 4 sensed -> drive until the second crossing line");
        state++;
      }
      break;
    case 12:
      if(bridge->event->isEventSet(5)){
        //go up the ramp
        loader->loadMission("02_move_to_the_ramp.mission", lines, &linecount);
        bridge->event->isEventSet(6);
        sendAndActivateSnippet(lines,linecount);
        printf("# case=%d event 5 sensed - robot go up the rump for 1m\n",state);
        state++;
      }
      break;
    case 13:
      if(bridge->event->isEventSet(6)){
        //go up the ramp
        loader->loadMission("03_up_the_ramp.mission", lines, &linecount);
        bridge->event->isEventSet(7);
        sendAndActivateSnippet(lines,linecount);
        printf("# case=%d event 6 sensed - robot go up the rump and stop on flat surface\n",state);
        state++;
      }
      break;
    case 14:
      if(bridge->event->isEventSet(7)){
        //go up the ramp
        loader->loadMission("04_sweep.mission", lines, &linecount);
        bridge->event->isEventSet(8);
        sendAndActivateSnippet(lines,linecount);
        printf("# case=%d event 7 sensed - go to the hole and start sweeping\n",state);
        state++;
      }
      break;
    case 15:
        if(bridge->event->isEventSet(8)){
          loader->loadMission("05_grab_second_ball.mission", lines, &linecount);
          bridge->event->isEventSet(1);
          sendAndActivateSnippet(lines, linecount);
          printf("@case=%d - event 8 sensed -> go to catch the second ball\n",state);
          state++;
        }
        break;
    case 16:
      if(bridge->event->isEventSet(1)){
        loader->loadMission("06_place_second_ball.mission", lines, &linecount);
        bridge->event->isEventSet(9);
        sendAndActivateSnippet(lines, linecount);
        printf("@case=%d - go to place the second ball\n",state);
        state++;
      }
      break;
    case 17:
        if(bridge->event->isEventSet(9)){
          loader->loadMission("07_go_to_steps.mission", lines, &linecount);
          bridge->event->isEventSet(10);
          sendAndActivateSnippet(lines, linecount);
          printf("@case=%d - event 9 sensed -> go to steps \n",state);
          state++;
        }
        break;
    case 18:
        if(bridge->event->isEventSet(10)){
          //loader->loadMission("stop.mission", lines, &linecount);
          //bridge->event->isEventSet(3);
          //sendAndActivateSnippet(lines, linecount);
          //printf("@case=%d - event 10 sensed -> stop mission\n",state);
          state++;
        }
        break;    
    case 19:
      //if(bridge->event->isEventSet(3)){
        state=999;
        //printf("@case=%d robot stopped -> stop mission\n",state);
      //}

    case 999:
    default:
      printf("mission 2 ended \n");
      bridge->send("oled 5 \"mission 2 ended.\"");
      finished = true;
      play.stopPlaying();
      break;
  }
  // printf("# mission1 return (state=%d, finished=%d, )\n", state, finished);
  return finished;
}



/**
 * Run mission
 * \param state is kept by caller, but is changed here
 *              therefore defined as reference with the '&'.
 *              State will be 0 at first call.
 * \returns true, when finished. */
bool UMission::mission3(int & state)
{
  bool finished = false;
  int linecount = 0;
  switch (state)
  {
    case 0:
      printf("# started mission 3.\n");
      play.say("Mission 3 - climb down stairs", 90);
      bridge->send("oled 5 mission 3 started");
      state=11;
      break;
    case 11:
      loader->loadMission("01_stairs.mission", lines, &linecount);
      bridge->event->isEventSet(1);
      sendAndActivateSnippet(lines, linecount);
      printf("# case=%d START STAIRS", state);
      state = 20;
      
      break;   
    case 20:
      if(bridge->event->isEventSet((1))){
        loader->loadMission("02_roundabout.mission", lines, &linecount);
        bridge->event->isEventSet(2);
        sendAndActivateSnippet(lines, linecount);
        printf("# case=%d event 1 sensed -> stairs done - go to roundabout", state);
        //state++;
        state++;
      }
      break;
    case 21:
    if(bridge->event->isEventSet((2))){
        printf("# case=%d event 2 sensed -> roundabout done", state);
        //state++;
        state = 999;
      }
      break;
    case 22:
      loader->loadMission("stop.mission", lines, &linecount);
      bridge->event->isEventSet(3);
      sendAndActivateSnippet(lines, linecount);
      printf("# case=%d  STOP MISSION", state);
      state++;
      break;
    case 23:
      if(bridge->event->isEventSet(3)){
        state=999;
        printf("@case=%d robot stopped\n",state);
      }

    case 999:
    default:
      printf("mission 3 ended \n");
      bridge->send("oled 5 \"mission 3 ended.\"");
      finished = true;
      play.stopPlaying();
      break;
  }
  // printf("# mission1 return (state=%d, finished=%d, )\n", state, finished);
  return finished;
}


/**
 * Run mission
 * \param state is kept by caller, but is changed here
 *              therefore defined as reference with the '&'.
 *              State will be 0 at first call.
 * \returns true, when finished. */
bool UMission::mission4(int & state)
{
    bool finished = false;
    int linecount = 0;
    float v = 0.2; //driving speed
    float wallD = 0.2; //Distance from wall to track
    float wallThreshold = 0.5; //Threshold distance for wall tracking

    //Corner parameters
    float L = 0.1; //Distance between center and wheel axels
    float E = 0.285; //Length of extending piece of front door
    float d = 0.20; //Distance from final position to box
    float s = 0.15; //Distance from final position to door
    float WB = 0.235; //Wheelbase
    float sideToCenter = 0.04; //Distance from side IR sensor to center of wheelbase
    float turnRadius = WB / 2 + wallD - sideToCenter;//Turn radius
    float cornerDistance = L; //Extra distance to drive after tracking wall
    float turnAngle = -85; //90 degree turn
    float upDist = L + s; //Distance to drive up after first turn
    switch (state)
    {
    case 0:
    {
        loader->loadMission("01_tunnel_aproach.mission", lines, &linecount);
        bridge->event->isEventSet(1);
        sendAndActivateSnippet(lines, linecount);
        state++;
        break;
    }
    case 1:
    {//Move next to tunnel
        if (bridge->event->isEventSet(1)) {
            loader->loadMission("02_tunnel.mission", lines, &linecount);
            printf("End load mission\n");
            float params[] = { v, wallD + 0.05, wallThreshold, cornerDistance, turnRadius + 0.05, turnAngle, upDist, d, WB / 2 }; //Vel, wall tracking distance, wall tracking stop distance, extra distance, turn radius, turn angle
            printf("Start formatting mission\n");
            loader->formatMission(lines, formatOutput, linecount, params);
            printf("Formatting mission end\n");
            bridge->event->isEventSet(1);
            printf("Event 1 set\n");
            sendAndActivateSnippet(formatOutput, linecount);
            printf("mission sent\n");
            state++;
        }

        break;
    }
    case 2:
    {//Open front door and go through tunnel
        if (bridge->event->isEventSet(1)) {
            loader->loadMission("03_tunnel_open.mission", lines, &linecount);
            loader->clearOutput(formatOutput, 20);
            float params[] = { turnAngle, d, WB / 2 };
            loader->formatMission(lines, formatOutput, linecount, params);
            bridge->event->isEventSet(2);
            sendAndActivateSnippet(formatOutput, linecount);
            state++;
        }
        break;
    }
    case 3:
    {//Close front door
        if (bridge->event->isEventSet(2)) {
            loader->loadMission("04_tunnel_close.mission", lines, &linecount);
            bridge->event->isEventSet(3);
            sendAndActivateSnippet(lines, linecount);
            state++;
        }
        break;
    }
    case 4:
    {//Get next to tunnel. Same method as before
        if (bridge->event->isEventSet(3)) {
            loader->loadMission("02_tunnel.mission", lines, &linecount);
            loader->clearOutput(formatOutput, 20);
            float params[] = { v / 2, wallD, wallThreshold, cornerDistance - 0.1, turnRadius, turnAngle, 0.1, d, WB / 2 }; //Vel, wall tracking distance, wall tracking stop distance, extra distance, turn radius, turn angle
            loader->formatMission(lines, formatOutput, linecount, params);
            bridge->event->isEventSet(1);
            sendAndActivateSnippet(formatOutput, linecount);
            state++;
        }
        break;
    }
    case 5:
    {//Close second door
        if (bridge->event->isEventSet(1)) {
            loader->loadMission("05_tunnel_close_second.mission", lines, &linecount);
            bridge->event->isEventSet(4);
            sendAndActivateSnippet(lines, linecount);
            state++;
        }
        break;
    }
    case 6:
    {
        if (bridge->event->isEventSet(4)) {
            loader->loadMission("06_axe.mission", lines, &linecount);
            bridge->event->isEventSet(5);
            sendAndActivateSnippet(lines, linecount);
            state++;
        }
        break;
    }
    case 7:
    {
        if (bridge->event->isEventSet(5)) {
          loader->loadMission("07_go_home.mission", lines, &linecount);
          bridge->event->isEventSet(6);
          sendAndActivateSnippet(lines, linecount);
          printf("Axe completed! Go home\n");
          state++;
        }
        break;
    }
    case 8:
      if(bridge->event->isEventSet(6)){
        printf("Reached GOAL\n");
        state++;
      }
      break;
    default:
        printf("mission 4 ended\n");
        bridge->send("oled 5 mission 4 ended.");
        finished = true;
        break;
    }
    return finished;
}


void UMission::openLog()
{
  // make logfile
  const int MDL = 32;
  const int MNL = 128;
  char date[MDL];
  char name[MNL];
  UTime appTime;
  appTime.now();
  appTime.getForFilename(date);
  // construct filename ArUco
  snprintf(name, MNL, "log_mission_%s.txt", date);
  logMission = fopen(name, "w");
  if (logMission != NULL)
  {
    const int MSL = 50;
    char s[MSL];
    fprintf(logMission, "%% Mission log started at %s\n", appTime.getDateTimeAsString(s));
    fprintf(logMission, "%% Start mission %d end mission %d\n", fromMission, toMission);
    fprintf(logMission, "%% 1  Time [sec]\n");
    fprintf(logMission, "%% 2  mission number.\n");
    fprintf(logMission, "%% 3  mission state.\n");
  }
  else
    printf("#UCamera:: Failed to open image logfile\n");
}

void UMission::closeLog()
{
  if (logMission != NULL)
  {
    fclose(logMission);
    logMission = NULL;
  }
}
