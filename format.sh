#!/bin/bash

black tricebot.py
astyle --style=kr src/*.c src/*.h tests/*.cpp tests/*.c tests/*.h
