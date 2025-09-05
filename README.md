
# Drops

![License](https://img.shields.io/badge/License-GPLv3-blue.svg)

## What

Drops is a lightweight Windows screensaver designed to be visually pleasing and compatible with a wide range of Windows versions.


## Why

1. On Windows PCs, screensavers are one of the few areas where users can implement custom code and visuals.
2. To learn about the Windows API and explore the inner workings of screensavers.
3. Windows XP was the first version fully based on the WinNT architecture, eliminating dependence on MS-DOS.


## Features

- **Wide compatibility**: Works on Windows XP, Vista, 7, 8, 8.1, 10, and 11 (32 & 64-bit).
- **CPU-Only Rendering**: No GPU or hardware acceleration required.
- **Customizable**: Edit displayed text, graphics frequency, and more to tailor the experience.


## Build

Drops was developed in Visual C++ 2008 on Windows XP using C++03 standards.

To build the project yourself:

1. Recreate the original development setup.
2. Open the project in Visual C++ 2008.
3. Compile and build the screensaver executable.
4. Change the extension of the *.exe to *.scr


## Usage instructions

Place the Drops.scr file inside the C:\Windows\System32 folder

Screensavers store their settings in the Windows Registry.
The path depends on your Windows version:
(Vista and later): HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Screensavers
(XP and earlier): HKEY_CURRENT_USER\Software\Microsoft\Screensavers

The method to change the current screensaver depends on your Windows version:
(XP, Vista): Use the Control Panel → Display → Screen Saver settings.
(Windows 7 and later): Search for Screen Saver in the Start menu or Settings.

---

![C++03](https://img.shields.io/badge/C%2B%2B-03-blue.svg)
![Windows XP](https://img.shields.io/badge/Windows%20XP-003399?logo=windows-xp&logoColor=white)
![WinAPI](https://img.shields.io/badge/WinAPI-API-blue.svg)
![Visual C++ 2008](https://img.shields.io/badge/Visual%20C%2B%2B%202008-5C2D91?logo=visual-studio&logoColor=white)


