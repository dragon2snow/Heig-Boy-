@echo off
del testprog.gb
del testprog.obj
set path=..\tools;%path%
rgbasm95 -otestprog.obj -iinclude\ testprog.asm
if not exist testprog.obj (
	pause
	exit
)
xlink95 testprog.lnk
rgbfix95 -v testprog.gb
del testprog.obj
..\tools\VisualBoyAdvance testprog.gb
