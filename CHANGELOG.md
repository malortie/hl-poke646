# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [2.0.0] - 2021-11-07

### Added

- CMake support

### Changed

- Moved CBradnailer to CGlock
- Moved CCmlwbr to CCrossbow
- Moved CHeaterPipe to CCrowbar
- Moved CNailgun to CMP5
- Moved CPipeBomb to CSatchel
- Moved CXenSquasher to CGauss
- Moved BULLET_PLAYER_NAIL to BULLET_PLAYER_9MM
- Moved nails ammo to 9mm
- Reworked player exert and breathe code
- Reworked bradnailer code
- Reworked cmlwbr code
- Reworked shotgun primary attack code
- Reworked pipebomb pickup code
- Reworked fire trail code
- Reworked barnacle ammo xencandy spawn code
- Reworked xen spit and xen squasher code
- Reworked human grunt custom code
- Reworked generic model code
- Reworked robocop code
- Reworked HUD health code
- Reworked HUD scope code
- Reworked HUD weapon list code
- Reworked MP3 management code
- Tweaked nail life time
- Tweaked shotgun fire impulse
- Tweaked shotgun fire viewpunch
- Tweaked shotgun reload delays
- Tweaked wall smoke particle effects
- Renamed nail entity class to nailgun_nail
- Renamed ammo nails to nail
- Made nails stay at random offset when hitting world
- Made xen squasher play idle animation
- Occasionally emit sparks at nail hit location
- Emit bubbles when nail is travelling underwater

### Removed

- cl_upspeed fixed value
- Hardcoded map fixes
- item_suit give on impulse 101

### Fixed

- Wrong player breathe sound used
- Ammo limit sent to client
- Wrong time base used for weapons next attack times
- Shotgun pump sound playing twice upon finishing reload
- Shotgun ejecting only 1 shell
- Pipebomb sequence out of range
- Incorrect spawn objects in func_break.cpp
- Generic model calling wrong base class in KeyValue
- Crowbar not playing the first attack hit sequence
- Nail emitting sparks when touching sky surfaces

## [1.0.0] - 2016-02-01

### Added

- Poke646 reimplementation source code
