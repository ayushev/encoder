# Encoder
This command line application takes as an argument a directory with WAVE files and convert them into MP3 files

## System requirements (Linux)
* [LAME](http://lame.sourceforge.net/) mp3 library (e.g. Debian: sudo apt-get install libmp3lame0)
* [Python 2.7](https://www.python.org/) for SCons (e.g. Debian: sudo apt-get install python2.7)
* [SCons](http://scons.org/) building tool (e.g. Debian sudo apt-get install scons)

## System requirements (Windows)
* [Cygwin](https://www.cygwin.com/) toolset - [download](https://cygwin.com/install.html)
* [Python 2.7](https://www.python.org) for SCons - [download](https://www.python.org/downloads/)
* [SCons](http://scons.org/) building tool - [download](http://prdownloads.sourceforge.net/scons/scons-2.5.0-setup.exe)
* [LAME](http://lame.sourceforge.net/) mp3 library. If you don't have those libraries preinstalled, you can built them from sources on your machine with Cygwin/MinGW/MSVC.

## Features
* PCM 8/16/24/32 bps (bits per sample) 
* MP3 with VBR (variable bitrate)
* Parallel files processing via POSIX threads

## Usage
1. Download zip from github or clone the repository (you need to have a github application on your system for this)
2. In console/terminal_emulator type: `cd <encoder_folder>`
3. `scons` . [Windows only] If your static LAME library is in a place where scons can't find it, you may give to scons --lamepath=<path_to_library> option to point the proper place
4. `./build/encoder[.exe] [-th] test/` Where `-t` option specifies how much threads you want to allow to use.

## Test folder
In test folder you can find files in the folowing format XXYYa.wav, where
* `XX` - Bits Per Sample in PCM, can be 08/16/24/32
* `YY` - Bitrate, can be 8000 Hz/11500 Hz/ 22050 Hz/ 44100 Hz
* `a`  - Mono/Stereo
