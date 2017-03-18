-- gpSP-Nspire  Gameboy Advance emulator for TI-Nspire CX --
Alpha v0.11, released August 6,2012

Based on the source code of gpSP2X for GP2X/WIZ by Exophase and notaz.
Ported to TI-Nspire CX by calc84maniac.

-- Quick feature list --
   * 4 screen scaling modes
   * Flexible frameskip options, per-game settings
   * Fully remappable controls
   * 10 savestate slots
   * Quick save/exit button
   * Loads zipped roms
   * Limited cheat support

-- Version history --
Alpha v0.11 (2012/8/6):
   * First ticalc.org release.
   * Set LCD speed to 60Hz to improve overall timing.
   * Add unblended scaling option.
   * Fix glitch where BIOS error messages are not displayed.
   * Allow browsing the entire filesystem for ROMs.
   * Let the ingame menu key also exit the menu.
   * Turn on alignment exceptions before exit.
   * Disable document screen refresh on exit, it made exiting too slow.
Alpha v0.10 (2012/8/2):
   * First release, intended only for testing by Omnimaga users.


-- Main differences between gpSP-Nspire and gpSP2X --
1) All files sent to the TI-Nspire must have a .tns file extension.
   According to convention, this extension should be added in addition to the original extension.
   On Windows, you may need to change your folder settings if you cannot see file extensions.
   Examples:
     gba_bios.bin -> gba_bios.bin.tns
     Super_Mario_Advance.gba -> Super_Mario_Advance.gba.tns
     Super_Mario_Advance.cht -> Super_Mario_Advance.cht.tns

2) Quick Save feature was added.
   This feature allows you to save the state to the current slot
   and quit instantly with the press of a button.

3) ROM list is not sorted alphabetically.
   This is due to a missing qsort function, and may be implemented in future versions.

4) Sound emulation is permanently disabled.
   This is a no-brainer since the Nspire CX cannot output sound.
   I may eventually try to support connecting a USB sound card, but no promises.

5) Savestate menu was removed. All savestate operations can be done from the main menu.
   Load Savestate From File currently does not work, and isn't very useful anyway.

6) Overclocking options were removed. Use an external program such as Nover to overclock.
   Note: If you set the AHB clock to a value over 70MHz, you are responsible for any problems.

7) Memory handling in the dynamic recompiler was optimized, which may or may not cause
   extra game compatibility issues. No such issues have been found at this time.


-- Installation --
1) If you do not have a TI-Nspire CX, this emulator is not compatible. Don't bother.

2) Install the latest version of Ndless on your CX if you don't have it already.

3) Send gpsp_launcher.tns, gpsp_resources.tns, and game_config.txt.tns to your CX,
   all in the same folder.

4) Place your GBA BIOS in the folder from step 3. This file must be named
   "gba_bios.bin.tns" in all lowercase as shown, so rename it if needed.

   -- NOTE --

   There are two commonly available BIOSes - one is the correct one used in
   production GBA's worldwide and the other is a prototype BIOS. The latter
   will not cause some games to not work correctly or crash. If you attempt
   to use this BIOS you will be presented with a warning before being allowed
   to continue. This screen will give you a checksum of the real BIOS image.
   Do not ask me where to find a copy, I cannot legally tell you.

5) Send your GBA games to your CX, which must have the ".gba.tns" file extension.
   These may be placed in their own folder, or in the same folder as gpSP-Nspire.
   Zip compressed ROMs are also supported, by sending a ROM to a zip folder and
   changing the zip folder's extension to ".zip.tns".

6) Open gpsp_launcher.tns. You can now select a ROM from the left or navigate folders on the right.
   gpSP-Nspire is not a perfect emulator, and not all games work properly. Live with it.


-- Key layout --

Menu keys:
  Move - Up/down on Touchpad, or 8/5 on numpad
  Change options - Left/right on Touchpad, or 4/6 on numpad
  Select - Click or enter
  Go back - menu (or assigned menu key, see below)
  Move up (in directory listing) - esc

Default ingame keys (can be changed in the menu):
Note, touchpad always acts as d-pad.
  D-pad up      8
  D-pad down    5
  D-pad left    4
  D-pad right   6
  A             del
  B             var
  Left Trigger  scratch
  Right Trigger doc
  Start         shift
  Select        ctrl
  Menu          menu
  Fast Forward  F
  Load State    L
  Save State    S
  Save+Exit     Q


-- Cheats --

Currently, gpSP supports some functionality of Gameshark/Pro Action Replay
cheat codes. To use these, you must first make a file with the same name
as the ROM you want the cheat code to apply to, but with the extension ".cht.tns".
Put this file in the same directory as the ROM. To make it use a normal
text editor like notepad or wordpad if you're on Windows.

To write a code, write the type of model it is, gameshark_v1, gameshark_v3,
PAR_v1, or PAR_v3. gameshark_v1/PAR_v1 and gameshark_v3/PAR_v3 respectively
are interchangeable, but v1 and v3 are not! So if you don't know which
version it is, try both to see if it'll work.

Then, after that, put a space and put the name you'd like to give the cheat.

On the next several lines, put each cheat code pair, which should look like
this:

AAAAAAAA BBBBBBBB

Then put a blank line when you're done with that code, and start a new code
immediately after it. Here's an example of what a cheat file should look
like:


gameshark_v3 MarioInfHP
995fa0d9 0c6720d2

gameshark_v3 MarioMaxHP
21d58888 c5d0e432

gameshark_v3 InfHlthBat
6f4feadb 0581b00e
79af5dc6 5ce0d2b1
dbbd5995 44b801c9
65f8924d 2fbcd3c4

gameshark_v3 StopTimer
2b399ca4 ec81f071


After you have written the .cht.tns file, you have to enable the cheats
individually in the game menu. Go to the Cheats/Misc menu, and you will
see the cheats; turn them on here. You may turn them on and off as you
please, but note that some cheats may still hold after you turn them off,
due to the nature of the system. Restart to completely get rid of them.

IMPORTANT NOTES:

Not all of gameshark's features are supported, especially for v3. Only
basic cheats will work, more or less.

Cheats may be unstable and may crash your game. If you're having problems
turn the cheats off.

Really, there's no guarantee that ANY cheats will work; I tried a few and
some seem to work, others are questionable. Try for yourself, but don't
expect anything to actually work right now.


-- Thanks --
Of course, thanks to Exophase for all the hard work he put into the original gpSP.
Also, thanks to notaz for maintaining the gp2X/Wiz version and adding new features.
Many thanks to tangrs for making the ELF loader that I modified for use with gpSP.
Thanks to annoyingcalc for playtesting and finding bugs.
And thanks to the entire Omnimaga community for supporting me.