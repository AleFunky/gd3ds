# Geometry Dash for the 3DS

This is a demake of the popular mobile game **Geometry Dash** for the **Nintendo 3DS**.

Geometry Dash and its respective resources are by RobTop Games.
<img width="1460" height="480" alt="Main Menu" src="https://github.com/user-attachments/assets/d802266b-2eb9-4115-870c-0073ed5a50ff" />
<img width="1460" height="480" alt="Icon Kit" src="https://github.com/user-attachments/assets/d55b2434-4f4b-4136-bab8-668f5e8e62c7" />
<img width="1460" height="480" alt="Gameplay" src="https://github.com/user-attachments/assets/cec39d8a-0326-4924-a8f0-20c90dc403fa" />
<img width="1460" height="480" alt="Pause Menu" src="https://github.com/user-attachments/assets/05b43079-c36e-4ce9-a433-61bd6dbfb9c8" />


## Features
### **In the current release**:
- [x] All main levels up to Theory of Everything 2
- [x] Gameplay features up to GD 1.9
- [x] Accurate physics
- [x] Accurate visuals and FX
- [x] Custom level importing (via SD Card)
- [x] Click-On-Steps input
- [x] Practice mode
- [x] Many QOL features (Practice music sync, show hitboxes)
- [x] Icons up to GD 2.11 
- [x] Progress tracking and Statistics
- [x] Stereoscopic 3D support
### **Coming Soon**:
- [ ] Online level downloading from both the main servers and 1.9 GDPS
- [ ] Icons up to GD 2.2
- [ ] Start position support
### **Other potential features**:
- [ ] Achievements
- [ ] Better menu navigation w/o touchscreen
- [ ] More VFX
- [ ] More QOL/Cheat features (StartPos switcher, Noclip accuracy, etc.)

## Credits
 - __RobTop Games__ - Geometry Dash
 - __AleFunky__ - Lead Developer
 - __camila314__ - Pathfinder Mod's physics
 - __advexed__ - UI, VFX, Online
 - __nittynatty__ - VFX
 - __orionconstel__ - Concepts, Menus
 - __cloud54__ - UI
 - __novex__ - Optimizations
 - __DiegoWarden__ - 240hz input
 - __zylonity__ - Stereoscopic 3D support
 - __Crafty Jumper__ - UI Assets

## Download
The current release is available on Universal Updater. Alternatively, you can manually download both the .3dsx and the .cia files [here](https://github.com/AleFunky/gd3ds/releases/latest), or you can scan the QR code below in FBI to install the game to your home menu automatically.\
<img width="256" height="256" alt="imagen" src="https://github.com/user-attachments/assets/0df1a8b2-f653-41ff-a0ce-608d73cf54d1" />

# Discord
You can visit our Discord server and get support (or talk if you want to): [Discord](https://discord.gg/Yh6JrS7eSU)

# FAQ
### How do I install this on my 3DS?
The current easiest way is to open the Universal Updater app on your 3DS and download the game from there. If you prefer doing it manually, download either the .3dsx or .cia file from the releases page and place it on your SD Card, then depending on which file you chose, launch it through the Homebrew Launcher, or install it to your home menu through FBI.

### The game is saying something about "missing DSP firmware", what do I do?
If you're playing on actual hardware this shouldn't be an issue as most 3DS homebrew tutorials dump this file in the process. If you're playing on an emulator, navigate to ```(your emulator's data folder)\sdmc\3ds\``` and create a file named ```dspfirm.cdc``` in said location. It can be completely empty for all the emulator cares, it just has to be present. In case you're running into this error on a real console, open the Luma menu (usually accessed by pressing ``L``, ``DPAD DOWN`` and ``SELECT`` at the same time), scroll to ``Miscellaneous options...`` and press ``Dump DSP firmware``.

### How do I play / add custom levels?
You'll need to either export a copy of your level of choice using the [GDShare Geode mod](https://geode-sdk.org/mods/hjfod.gdshare) or download an archive of said level from [GDHistory](https://history.geometrydash.eu/). If the level uses a custom song, you'll also need to either extract it from your Geometry Dash songs folder (```%localappdata%\GeometryDash``` on Windows), or download it separately from [Newgrounds](https://www.newgrounds.com/audio). Once you have the level .gmd (and song, renamed to its Newgrounds ID) prepared, copy them to ```\3ds\gd3ds\external_levels\``` and ```\3ds\gd3ds\saved_songs\``` on your SD Card respectively. Putting the level files into additional folders within the main ```\external_levels\``` directory is supported. Do keep in mind, however, that any objects from updates 2.0 and above will not load, and object-heavy levels are not guaranteed to be playable - especially on non-New 3DS models.

### Are you going to add a level editor?
The answer for now is no. The idea of a level editor is something that is often thought over among the developers, especially in light of recent architectural improvements which would make its implementation more straightforward. While it's still not very likely of a feature due to its scope and questionable utility, it's not something that is immediately turned down. 

### Can you add X feature / X gamemode / X level?
The current scope of the game is to have all gameplay features and main levels up to 1.9. The game currently supports importing .gmd files through the SD card (see above), and online level downloading is actively in development. For all intents and purposes, the game is largely feature-complete. Quality of Life features outside of gameplay and level additions are welcome as suggestions, which you can mention in our Discord server. (Please do not ping or DM developers to tell them your ideas. The GD3DS channel is active enough for you to pop in and talk to the developers.)

### Why not add 2.0 / 2.1 / 2.2 / whatever?
Short answer: Performance concerns. An aspect of GD3DS that we do not want to leave behind is the support of the original 3DS systems. And, even then, many 1.9 levels still struggle to run on them. Updates 2.0+ only amplify these concerns. Moving objects are the main reason, as they require much more sophisticated trigger updating and per-frame math for potentially many, many objects at a time. Of course, both 2.0 and 2.1 are similar in this way. 2.2, naturally, blows those two out of the water. Not only is its feature set gargantuan, but many of its features are significantly more complex than even 2.1 systems, and it affects many more aspects of the game, such as physics, triggers, and rendering, making it effectively impossible to implement in full performantly and faithfully. 
