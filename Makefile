MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR := $(patsubst %/,%,$(dir ${MKFILE_PATH}))
BUILD_DIR := ${CURRENT_DIR}/tmp

SRC:=Attiny85CurrentMeter.ino
ALL_SRC:=$(wildcard *.ino *.h)

ELF:=tmp/${SRC}.elf
HEX:=tmp/${SRC}.hex

BASE:=/usr/share/arduino
USER_BASE:=$(HOME)/.arduino15
USER_LIBS:=$(HOME)/Arduino/libraries
# BOARD:=attiny:avr:ATtinyX5:cpu=attiny85,clock=internal8
BOARD:=ATTinyCore:avr:attinyx5:LTO=enable,TimerClockSource=default,chip=85,clock=8internal,eesave=aenable,bod=disable,millis=enabled
# -vid-pid=1A86_7523
HARDWARE:=-hardware ${BASE}/hardware -hardware ${USER_BASE}/packages 
TOOLS:=-tools ${BASE}/tools-builder -tools ${USER_BASE}/packages
LIBRARIES=-built-in-libraries ${BASE}/lib
LIBRARIES+=-libraries ${USER_LIBS}  # Where U8g2 comes from
WARNINGS:=-warnings all -logger human

ARDUINO_BUILDER_OPTS=${HARDWARE} ${TOOLS} ${LIBRARIES}
ARDUINO_BUILDER_OPTS+=-fqbn=${BOARD} ${WARNINGS}
ARDUINO_BUILDER_OPTS+=-verbose -build-path ${BUILD_DIR} 

${ELF}:	${ALL_SRC}
	@mkdir -p ${BUILD_DIR}
	arduino-builder -compile ${ARDUINO_BUILDER_OPTS} ${SRC} 2>&1 | tee build.log

tags:	${ALL_SRC}
	ctags -R . ${USER_LIBS} ${USER_BASE}

clean:
	rm -rf ${BUILD_DIR} build.log tags

upload:	${ELF}
	avrdude -C/home/ttsiod/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino9/etc/avrdude.conf -v -pattiny85 -cstk500v1 -P/dev/ttyUSB0 -b19200 -Uflash:w:${HEX}:i
	avr-size ${ELF}

stats:	${ELF}
	avr-nm --print-size -t d ${ELF} \
	    | c++filt | sort -n -k 2 | awk '{a+=$$2; print a " " $$0;}' | grep -v u8x8
	avr-size ${ELF}
