# PTZControl

## History
This small program is designed to control a Logitech PTZ 2 Pro. The camera was purchased to stream our church services.
Unfortunately, it quickly turned out that the operation with the remote control is possible but cumbersome and inaccurate. The camera was installed directly behind and above the video technician and always pointing back with the remote control was not comfortable. 
The control Logitech software shipped with the camera is completely worthless.
So, I made the decision to build a corresponding control program.
Some months later we bought a second camera, and the program was extended by the function to control another camera. At this time a maximum of two cameras was supported.
Later added support for the Logitech Rally, the PTZ Pro and other Logitech cameras. 
And finnaly as last feature the PTZcontrol now supports three cameras.

### Where is the basic code from?
It wasn't easy to get code that shows how to control a PTZ camera. 
Finally I contacted Logitech and I received download link to the `Logitech Collaboration Software Reference Kit` (Logitech CSRK)
I found some Lync remote control code in it, that gave me some hints and the guids and usage for the Logitect camera interface.

## Used interfaces
The program directly uses the camera media control via the Windows SDK (see links in the source code).
For controlling the zoom functions and the preset functions of the PTZ 2 Pro, Logitech provided me with some example code with defines from the Lync driver.
Logitech internal interfaces are used for:
- Saving the presets,
- Getting the presets
- For step-by-step Pan Tilt camera control (see also the settings dialog: Check "Use LogitechCamera Motion Control")

Standard controls from the Windows Driver API are used
- For zoom control
- For the timer-controlled PT (Pan Tilt) control (see Settings: no check mark at "Use Logitech Camera Motion Control")This control is the standard.

The program supports tooltips that and you can define them yourself to give the camera presets useful names. (Separate for each camera)

## Used environment and libraries
I used the Visual Studio 2019 Community Edition to develop this program with C++.
Using MFC and ATL as libraries. No additional software is used.
The EXE runs alone, without installing any other files or DLLs or any installation.

## Behaviour
The program is always in the foreground and has been designed relatively compact and small, so that you can hover somewhere over your OBS program and it is really easy to use.
Currently selected preset or home position are shown with a green background on the buttons.
Recalling a preset is simply done by clicking on one of the number buttons. 
Presets are changed by pressing the M button (Memory) followed by a number key. The M button turns red as long as it is active.

The program remembers its last position on the screen and is automatically repositioned to when it is started. 
All settings are stored in the registry under the branch `HKEY_CURRENT_USER\SOFTWARE\MRi-Software\PTZControl`.
See the command line secion too.

### Supported Cameras
Currently, the Logitech PTZ 2 Pro, PTZ Pro, Logitech Rally cameras, Logi Group Camera and ConferenceCam CC3000e Camera are automatically detected.
For other cameras, you can try to force detection by specifying the name (or part of the name) of the cameras in the registry or on the command line.

Internally, all cameras that have one of the following tokens in the name are automatically used:
- *PTZ Pro*
- *Logi Rally*
- *ConferenceCam*
- *Logi Group Camera*

### Guard Thread
Unfortunately, we have sometimes had the experience that OBS or the USB bus hangs with a camera. The PTZControl program then usually stops and stops responding because the camera control commands block the application.
Through an internal guard thread, the application can determine that it is no longer working correctly and terminates automatically.
Otherwise you would have to use the task manager and this can take a lot of time to terminate the application in the hustle and bustle of a livestream.

### Logitech Motion Control
The Logitech cameras have their own interface for pan/tilt control. This moves the camera in X/Y axes by a certain step value, This control is a special Logitech feature. 
If you click on a direction button once, exactly one step pulse is output.
If you hold down a direction button, a pulse is sent to the camera again and again at a certain interval to change the direction.

### Standard Motion Control
This control is the standard when you start the application for the first time.
I have made the experience that this Logitech motion control is a bit rough. Via the normal device control, a pan/tilt is also possible in corresponding motor commands for X/Y direction. This is done in turning on the motor for a specific time interval and turning it off again.
Accordingly, you can adjust the timer interval for Motor on/off accordingly. The default is 70msec. Values between 70 and 100 or goiod values.
If you click on a direction button once, the motor is turned on and off again after the corresponding interval.
If the direction button remains pressed, the motor remains switched on for the corresponding direction until the button is released again.
This control seems more effective and accurate to me and is the standard. The disadvantage is that if the timer interval is too small, the camera does not react immediately when a button is clicked. But since precision was more important to me because our camera is installed relatively far away from the podium, I use this setting with a 70msec timer.

## Normal Hotkeys
The program has serveral hotkeys that allows a control without the mouse when it has the focus.
- Pan-Tilt control with Left, Right, Up, Down keys.
- Home position with Num-0, Home keys.
- Memory function with the M-key.
- Recall stored position with the numeric keys 1-8 or the numeric key pad keys Num-1 to Num-8.#
- Open the setings dialog with Num-Divide or Num-Multiply
- Zoom in/out Page-Up/Down, Num+Plus, Num-Minus
- Select Camera 1. Alt+1, Alt+Num-1, Alt+Page-Up
- Select Camera 2. Alt+2, Alt+Num-2, Alt+Page-Down

## Global Hotkeys
There are also global hotkeys defined. They work from any active program. Normal hotkeys work only as long as the PTZControl program has the input focus. All global hotkeys use the Windows key in combination with Ctrl+Alt key. The global hotkeys defined are:
- For camera 1
	- Home position => Windows-Key + 0 (on the numeric key pad) 
	- Memory Position 1-8 => Windows-Key + 1-8 (on the numeric key pad)
- For camera 2
	- Home position => Windows-Key + Ctrl + 0 (on the numeric key pad) 
	- Memory Position 1-8 => Windows-Key + Ctrl + 1-8 (on the numeric key pad)
- For camera 3
	- Home position => Windows-Key + Alt + 0 (on the numeric key pad) 
	- Memory Position 1-8 => Windows-Key + Alt + 1-8 (on the numeric key pad)
If camera 2/3 are not available the hotkeys are ignored and not defined.


## Command Line Options

Several options can be specified on the command line. Command-line options override the corresponding settings stored in the registry.

| Option                    | Description                                                                                                                                                                                                                      |
| ------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `-device:"<device name>"` | Specifies a camera by matching part of its device name. If `"*"` is specified, any detected camera is accepted.                                                                                                                  |
| `-showdevices`            | Displays a message box after startup showing the names of all detected cameras.                                                                                                                                                  |
| `-noreset`                | By default, a detected camera is moved to its home position (Logitech preset) and the zoom is reset to the maximum wide-angle setting during startup. If `-noreset` is specified, the camera position and zoom remain unchanged. |
| `-noguard`                | Prevents the application from performing a controlled self-termination. This option can be useful for debugging and testing, especially when investigating software defects.                                                     |

## Controlling Cameras via the Command Line

Applications such as *Bitfocus Companion* and *Elgato Stream Deck* can launch external programs. The extended command-line options make it possible to control cameras directly from these devices.

| Option         | Description                                                |
| -------------- | ---------------------------------------------------------- |
| `-<n>`         | Select camera *n* (`1`, `2`, or `3`).                      |
| `-zoom_in`     | Zoom the camera in.                                        |
| `-zoom_out`    | Zoom the camera out.                                       |
| `-move_up`     | Tilt the camera upward.                                    |
| `-move_down`   | Tilt the camera downward.                                  |
| `-move_left`   | Pan the camera to the left.                                |
| `-move_right`  | Pan the camera to the right.                               |
| `-move_home`   | Move the camera to its home position.                      |
| `-store:<n>`   | Store the current camera position in preset *n* (`1`–`8`). |
| `-restore:<n>` | Restore the camera position from preset *n* (`1`–`8`).     |

Only one camera can be controlled per command-line invocation. To control multiple cameras, invoke the application multiple times.

Multiple commands can be combined in a single command line. Regardless of the order in which the options are specified, commands are always executed in the following sequence:

**Home → Restore → Zoom → Pan → Tilt → Store**

### Internal handling

When *PTZControl* is already started the applications starts normally and executes commands that might be defined on the command line.

When *PTZControl* is already started, the command line options are examined and the command is transferred to the already running instance of the command.

Upon successful of a command line execution the exe return code is 0.<br>
When the syntax of a command line is incorrect the return code of the exe is 8.<br>
When the a command can't be executed the return code of the exe is 16.<br>



## Registry settings
In the registry branch `HKEY_CURRENT_USER\SOFTWARE\MRi-Software\PTZControl\Options` it is possible to preset the following options  without using the command line.

**NoReset (DWORD value)**
*Value <>0:* Has the same function as -noreset on the command line. The current camera and zoom position is maintained when starting the program. Value = 0: When starting the program, you move to the home position and zoom to maximum wide angle. (Default)

**NoGuard (DWORD value)**
*Value <>0:* The guard thread that may automatically terminate the application is terminated.
*Value = 0:* The guard thread automatically terminates the application if a blocking of the USB bus is detected. (Default)

 
