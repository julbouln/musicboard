#!/bin/bash

git clone git://git.code.sf.net/p/openocd/code openocd-stm32f7 || exit 1
cd openocd-stm32f7 || exit 1
git fetch http://openocd.zylin.com/openocd refs/changes/54/2754/2 && git checkout FETCH_HEAD || exit 1
git fetch http://openocd.zylin.com/openocd refs/changes/55/2755/4 && git cherry-pick FETCH_HEAD || exit 1
./bootstrap || exit 1
./configure --enable-stlink || exit 1
make -j4 || exit 1
