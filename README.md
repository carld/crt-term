[![Build Status](https://travis-ci.org/carld/og-term.png?branch=master)](https://travis-ci.org/carld/og-term)

# Terminal Window

A libtsm based terminal emulator that renders a BDF font to an OpenGL texture.

Uses shaders to simulate CRT monitor.

![Screenshot](/screen02.png?raw=true)

# Installation

    git clone https://github.com/carld/og-term.git
    git submodule init
    git submodule update
    make

# Options

    ./og-term [-f bdf font file] [-s fragment shader file]

