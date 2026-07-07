#!/bin/sh

sudo add-apt-repository ppa:stsp-0/dj64
sudo add-apt-repository ppa:dosemu2/ppa
sudo add-apt-repository -y ppa:jwt27/djgpp-toolchain
sudo apt install -y djstub dj64-host dos2unix gcc-djgpp djgpp-dev
