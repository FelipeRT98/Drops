# Drops

![License](https://img.shields.io/badge/License-GPLv3-blue.svg)


## What

Drops is a lightweight Windows screensaver designed to be visually pleasing and compatible with a wide range of Windows versions.

![Drops_256_16_4.png](.github/Drops_256_16_4.png)


## Features

- **Wide compatibility**: Works on Windows XP, Vista, 7, 8, 8.1, 10, and 11 (32 & 64-bit).
- **CPU-Only Rendering**: No GPU or hardware acceleration required.
- **Customizable**: Edit displayed text, graphics frequency, and more to tailor the experience.


## Why

- I wanted to make something for Windows, and screensavers are one of the few areas of Windows where I can implement my own code.

- I was interested in learning about low-level platform-specific toolkits, exploring Windows features and creating software compatible with multiple Windows versions.

- (Windows XP, Visual C++ 2008, WinAPI, C++03) – This combination let me achieve my goals. XP was the first version fully based on the WinNT architecture, removing dependence on MS-DOS.


## Build

Drops was developed in Visual C++ 2008 on Windows XP using C++03.

To build the project yourself:

1. Recreate the original development setup.
2. Open the project in Visual C++ 2008.
3. Compile and build the screensaver executable.
4. Change the extension of the Drops.exe to Drops.scr


## Usage instructions

To **preview** the screensaver, right-click on Drops.scr and select Test.

To **set** it as the screensaver:

1. Place the Drops.scr file inside the C:\Windows\System32 folder.
2. Choose the screensaver:
   - **(XP, Vista)**: Use the Control Panel → Display → Screen Saver settings.
   - **(Windows 7 and later)**: Search for Screen Saver in the Start menu or Settings.


---


![C++03](https://img.shields.io/badge/C%2B%2B-03-blue.svg)
![Windows XP](https://img.shields.io/badge/Windows%20XP-003399?logo=windows-xp&logoColor=white)
![WinAPI](https://img.shields.io/badge/WinAPI-API-blue.svg)
![Visual C++ 2008](https://img.shields.io/badge/Visual%20C%2B%2B%202008-5C2D91?logo=visual-studio&logoColor=white)
