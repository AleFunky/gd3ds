# Geometry Dash for the 3DS

This is a demake of the popular mobile game **Geometry Dash** for the **Nintendo 3DS**.

Geometry Dash and its respective resources are by RobTop Games.
<img width="1460" height="480" alt="imagen" src="https://github.com/user-attachments/assets/631a3a8b-1750-4f18-b534-9b7ce75e6746" />
<img width="1460" height="480" alt="imagen" src="https://github.com/user-attachments/assets/d55b2434-4f4b-4136-bab8-668f5e8e62c7" />
<img width="1460" height="480" alt="imagen" src="https://github.com/user-attachments/assets/7f6d7c17-071d-49a3-8433-6fe2ae13fe8a" />
<img width="1460" height="480" alt="imagen" src="https://github.com/user-attachments/assets/6508e8d3-8114-4035-836e-675813fc13f1" />


## Features
- [x] Main levels up to Theory of Everything 2
- [x] Gameplay
- [x] Accurate physics
- [x] Hitbox display
- [x] Particles
- [x] Player trails
- [x] Practice mode
- [x] Practice music sync
- [x] Level coins
- [x] Settings and Icon Saving
- [x] Progress tracking and Statistics
- [x] 3D support
- [x] Dual-screen UI
- [x] 1.9 Custom Level Importing (via SD Card)
- [x] Bottom screen tap effects 
- [x] Icons on the Title Screen
- [x] Various in-game visual effects and UI animations
- [ ] Auto-Checkpoints
- [ ] Online support
- [ ] Start position support
- [ ] Better bottom screen HUD

## Features that aren't planned but may be added either way
- [ ] Achievements
- [ ] Touchscreen-less navigation options
- [ ] Dual death effect
- [ ] Size portal
- [ ] Portal flash effect upon use
- [ ] Gravity switch effect
- [ ] Startpos switcher
- [ ] Cheat display

## Additional Credits
 - __camila314__ - Pathfinder Mod's physics

## Download
The current release is available on Universal Updater. Alternatively, you can manually download both the .3dsx and the .cia files [here](https://github.com/AleFunky/gd3ds/releases/latest), or you can scan the QR code below in FBI to install the game to your home menu automatically.\
<img width="256" height="256" alt="imagen" src="https://github.com/user-attachments/assets/0df1a8b2-f653-41ff-a0ce-608d73cf54d1" />

# Discord
You can visit our Discord server and get support (or talk if you want to): [Discord](https://discord.gg/Yh6JrS7eSU)

# FAQ
### How do I install this on my 3DS?
The current easiest way is to open the Universal Updater app on your 3DS and download the game from there. If you prefer doing it manually, download either the .3dsx or .cia file from the releases page and place it on your SD Card, then depending on which file you chose, launch it through the Homebrew Launcher, or install it to your home menu through FBI.

### The game is saying something about "missing DSP firmware", what do I do?
If you're playing on actual hardware this shouldn't be an issue as most 3DS homebrew tutorials dump this file in the process. If you're playing on an emulator, navigate to ```(your emulator's data folder)\sdmc\3ds\``` and create a file named ```dspfirm.cdc``` in said location. It can be completely empty for all the emulator cares, it just has to be present. In case the error is appearing on actual hardware, open the Luma menu (usually accessed by pressing ``L``, ``DPAD DOWN`` and ``SELECT`` at the same time), scroll to ``Miscellaneous options...`` and press ``Dump DSP firmware``.

### How do I play / add custom levels?
You'll need to either export a copy of your level of choice using the [GDShare Geode mod](https://geode-sdk.org/mods/hjfod.gdshare) or download an archive of said level from [GDHistory](https://history.geometrydash.eu/). If the level uses a custom song, you'll also need to either extract it from your Geometry Dash songs folder (```%localappdata%\GeometryDash``` on Windows), or download it separately from [Newgrounds](https://www.newgrounds.com/audio). Once you have the level .gmd (and song, renamed to its Newgrounds ID) prepared, copy them to ```\3ds\gd3ds\external_levels\``` and ```\3ds\gd3ds\saved_songs\``` on your SD Card respectively. Putting the level files into additional folders within the main ```\external_levels\``` directory is supported. Do keep in mind, however, that any objects from updates 2.0 and above will not load, and object-heavy levels are not guaranteed to be playable - especially on non-New 3DS models.

### Are you going to add a level editor?
The short answer is no. The long answer is that it's simply too much work for something that would run poorly on the already underpowered 3DS hardware, would be unable to upload levels to the Geometry Dash servers and that 99% of people would not care about. 

### Can you add X feature / X gamemode / X level?
Everything that's planned to be implemented is listed in the planned features section above - in short, anything major from updates above 1.9 will not be added. However, if you come up with an improvement, or a quality of life feature the game could use, you're welcome to suggest it in the Discord server.

### Why not add 2.0 / 2.1 / 2.2 / whatever?
Theoretically all features up to 2.1 ARE possible to implement, but they'd be a large undertaking to add in an adequately performant and accurate way. 2.2 only amplifies these concerns, not to mention it having some features that are fully impossible to have work faithfully on 3DS hardware (namely, shader triggers). Many 1.9 levels already struggle to run on the original 3DS, due to their high object counts, and future updates, with moving objects and much more decoration, exponentially worsen this issue. 
