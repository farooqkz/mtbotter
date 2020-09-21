# MtBotter

mtbotter is a library for creating simple or complex Minetest bots. You can move in
the world, add or remove blocks, punch nodes, mobs and players and receive currently a
few events such as removal or addition of a node, chat messages and time change.

About 80-90 % of the credit of this project goes to celeron55
and other authors and contributors of Minetest(engine).
I take the rest remaining credit!

To code a bot in C++(if you want to code your bot in other languages, see Other
languages section), see CREATE-BOT.md

## Other languages

Currently you don't have any option but C++.

## Build

Follow [Compiling instructions of Minetest](https://github.com/minetest/minetest#compiling).
The resulting library will appear in `/lib/` as `libmtbotter.so` on POSIX
systems(including Linux).

I haven't tested compiling it on other Operating Systems such as Windows or Mac OS X
but it should work.

## Todo

There are many files, functions, methods, classes and structures
 from Minetest which are not required by MtBotter such as GUI, Rendering, LocalDB,
 Sound and Server codes. They should be removed soon or late.

## Licence

MtBotter uses Minetest code and Minetest uses a copyleft licence named LGPL, thus
MtBotter is under the same licence(LGPL 2.1+).
For more information see LICENSE in project's root directory.
