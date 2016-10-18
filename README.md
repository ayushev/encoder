# Encoder
This command line application takes as an argument a directory with WAVE files and convert them into MP3 files

## System requirements (Linux)
* [LAME](http://lame.sourceforge.net/) mp3 library (e.g. Debian: sudo apt-get install libmp3lame0)
* [Python 2.7](https://www.python.org/) for SCons (e.g. Debian: sudo apt-get install python2.7)
* [SCons](http://scons.org/) building tool (e.g. Debian sudo apt-get install scons)

## System requirements (Windows)
* [MinGW](http://www.mingw.org/) toolset - [download](https://sourceforge.net/projects/mingw/files/latest/download?source=files)
** Packages mingw-developer-toolkit and mingw32-base should be seletced and installed as well 
** Add "<path_to_MinGW>\bin" path to environment variable PATH (e.g. "C:\MinGW\bin;")
* [Python 2.7](https://www.python.org) for SCons - [download](https://www.python.org/downloads/)
* Select checkbox to add Python in 
* [SCons](http://scons.org/) building tool - [download](http://prdownloads.sourceforge.net/scons/scons-2.5.0-setup.exe)
* C:\Python27\Scripts;C:\Python27;;

## Features
* PCM 8/16/24/32 bps (bits per sample) 
* MP3 with VBR (variable bitrate)
* Parallel files preocessing via POSIX threads

## Usage
1. Download zip from github or clone the repository (you need to have a github application on your system for this)
