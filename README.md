**LAMIAE IS CURRENTLY IN ALPHA**

**WHILE IT IS STABLE, AND IN A HIGHLY USABLE STATE, THINGS ARE IN A STATE OF SEMI-FLUX**


# What is Lamiae

Lamiae is a fork of Platinum Arts Sandbox meant to showcase my work on the RPG module since 2009.
Sandbox is in turn a fork of cube 2, which is a fantastic octree-based first person engine, naturally this isn't ideal for RPG-style gameplay, so getting to where we are now has been a significant challenge.
Cube 2 fork, Tesseract is also bundled, providing a state of the art lighting model and other advanced rendering features, including SSAO, HDR w/ tonemapping, Radiance Hints (Global Illumination) and more!

Lamiae builds upon cube 2 technologies, providing a highly flexible framework for RPGs more than capable of simulating many games from ages long past.
Check the "Inspiration" and the "Features" sections for a general idea.


# Inspiration

Note that these are taken at a cursory look at the games in question, with no knowledge of their actual inner workings.

Arcanum
* Dialogue Tree format - partially true, our dialogue nodes are named and use named destinations, not numbers.
* Status effects centred around polymorphing targets.
* Global flags and variables.

Fallout
* Trade/barter menu - similar layout
* Dual Wielding - Allowed you to have 2 items equipped and swap between then, basically where I got the idea for more or less the same system used in Skyrim.
* "Use" system - A "knife" could be used to both slash and thrust, Lamiae allows equal flexibility.

DnD
* Experience thresholds for levelling up.

Nethack
* Shopping - nethack allowed you to accrue store credit.

Diablo
* Item System - items from the same base can have wildly differing properties; They are always stacked where possible.

Oblivion
* A large number of status effect primitives, including ones which use arbitrary scripts
* Served as the inspiration for weapons with an effect for an area for an extended period of time.
* AI's Directives system; giving an NPC instructions and having them execute to the best of their ability, simultaneously if possible, and with prioritisation.


# List of Features

If you want a list of Sauerbraten's or Tesseract's respective features, visit their webpages.
Suffice to say cube 2 features a very easy to use WYSIWYG-style map-editor.

* Dedicated PC interface.
* Highly Performant - cube 2 and derived projects pride themselves on their fast and stable codebases.
* Stable enough for hardcore-roguelikes.
* Expansive RPG library built atop of cubescript with significant emphasis on flexibility and free-form mechanics.
* Signal based scripting system, allows both hard-coded and arbitrary named signals, very extensible.
* A simple yet flexible AI system which can be directed via directives and by tagging world objects (in progress).
* A wide variety of status effect primitives, including script types.
* A highly flexible template system for items, critters, triggers, objects, containers and more.
* No combat music
* Open Source and with Plain-Text, easy to modify formats - Makes modding a cinch.


# Origin of the name, "Lamiae"

The name "Lamiae" serves both as a joke and as a descriptor of what an RPG can do.
"Lamiae" itself is the plural form of "Lamia" - a creature with its origins in Greek Mythology.


1. The name literally means "Gullet," referring to one story in which Lamia was forced to eat her children by the goddess, Hera.
This refers to the tendency well made RPGs have for gobbling up huge chunks of your life.

2. Another story speaks of Lamia being traumatised at the death of her children, and experiencing the horrors again whenever she closed her eyes.
This refers to the large overall decline in RPG quality since 2003.
Unfortunately, RPG-fans have generally taken the decline rather well, preferring instead to point and laugh at the likes of Dragon Age 2, than roll for sanity whenever they reflect on the past.

3. Leading on from 2, another story tells that Zeus allowed her to remove her eyes to alleviate the trauma, this granted Lamia prophetic powers.
In a sense, Lamiae is my effort to try and promote certain RPG features I wish to see again in future games.
Lamiae is designed to encourage game designers to use skill checks, hefty dialogue trees, and choices with actual ramifications.

4. Another story tells that Lamia is a cross between a Succubus and a Vampire. This has a double intended meaning.
On one hand, you can look at it from the designer's perspective; He is entranced by his RPG project and will pour many years of his blood (, sweat, tears, and frustration) into it, so that it may be realised into its final form.
On the other, it also refers to the seductive quality of RPGs as a tool for escapism, and overindulging can have rather negative results on your future prospects, Succubi have level drain for a reason you know.


# Run Instructions

If you need to build a binary first, see the section below.

Once up and running, if you want to dive straight into making something, go to data/rpg/games and copy the "base" directory, this is the basis for your game.

You'll find additional readmes and instructions inside, as well as a [Tutorial on the wiki](https://github.com/Hirato/lamiae/wiki/Tutorial)

Good Luck!

## Windows
There is a lamiae.bat in the main directory, this is your sole means of launching lamiae.

If you're having issues running the game, make sure that your drivers are properly installed and that your GPU can at least match Intel's Sandy Bridge in all aspects.


## Linux/FreeBSD
There is a script named lamiae.sh in the main directory, running this will launch the proper binary for your platform.

A multitude of command line options are available, run with --help, -h or -? to get a list.

If you have problems running it, make sure that SDL2, SDL2_mixer and SDL2_image is installed. If you have an Nvidia GPU, you should also make sure that the proprietary drivers are installed and working.


## Mac (OS X) UNSUPPORTED
*Hopw Roewur Ne.*


# Build Instructions

## Windows
1. Install Codeblocks ( http://www.codeblocks.org/ ) - we **strongly** recommend the no-mingw version
2. Install mingw64 with SJLJ bindings ( http://tdm-gcc.tdragon.net/download ) - grab the tdm64-gcc bundle at the top
3. Open up the codeblocks project file in src/windows/lamiae.cbp
4. Click build

* You may need to manually specify the paths for mingw, just point it to C:\MingW64 (or where applicable) and replace the compiler executables with their generic selves (eg mingw32-i386-unknown-gcc.exe to gcc.exe, mingw32-i386-unknown-g++.exe to g++.exe)

**Official support is not provided for Visual Studio, but in theory the code should compile with 2005 and later.**


## Linux/FreeBSD
1. Install GCC
2. Install SDL2, sdl2_mixer and sdl2_image, as well as their *-dev counterparts
3. make -C src install

* if you want a debug binary instead, use "make -C src -f Makefile.debug install"
* If you're using BSD, you may need to specify the GNU version of the make utility.


## Macintosh (OS X) UNSUPPORTED
1. Install Xcode
2. Manually install SDL2, SDL2_image, and SDL2_mixer Frameworks.
3. Create, setup, and otherwise configure an xcode project for Lamiae.
4. ???
5. Profit.
