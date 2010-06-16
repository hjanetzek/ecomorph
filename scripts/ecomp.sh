#!/bin/sh

# Compiz Manager wrapper script
# 
# Copyright (c) 2007 Kristian Lyngstøl <kristian@bohemians.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
#
# Contributions by: Treviño (3v1n0) <trevi55@gmail.com>, Ubuntu Packages
#
# Much of this code is based on Beryl code, also licensed under the GPL.
# This script will detect what options we need to pass to compiz to get it
# started, and start a default plugin and possibly window decorator.
# 


COMPIZ_NAME="ecomorph" # Final name for compiz (compiz.real) 
GLXINFO="glxinfo"
# For Xgl LD_PRELOAD
#LIBGL_NVIDIA="/usr/lib/nvidia/libGL.so.1.2.xlibmesa"
#LIBGL_FGLRX="/usr/lib/fglrx/libGL.so.1.2.xlibmesa"

# Minimum amount of memory (in kilo bytes) that nVidia cards need
# to be allowed to start
# Set to 262144 to require 256MB
NVIDIA_MEMORY="65536" # 64MB
NVIDIA_SETTINGS="nvidia-settings" # Assume it's in the path by default

# For detecting what driver is in use, the + is for one or more /'s
XORG_DRIVER_PATH="/usr/lib/xorg/modules/drivers/+"

# Driver whitelist
WHITELIST="nvidia intel ati radeon i810 fglrx"

# blacklist based on the pci ids 
# See http://wiki.compiz-fusion.org/Hardware/Blacklist for details
#T="   1002:5954 1002:5854 1002:5955" # ati rs480
#T="$T 1002:4153" # ATI Rv350
#T="$T 8086:2982 8086:2992 8086:29a2 8086:2a02 8086:2a12"  # intel 965
#T="$T 8086:2e02 " # Intel Eaglelake
T="$T 8086:3577 8086:2562 " # Intel 830MG, 845G (LP: #259385)
BLACKLIST_PCIIDS="$T"
unset T

COMPIZ_OPTIONS=""
COMPIZ_PLUGINS="ini inotify"
ENV=""

# No indirect by default
INDIRECT="no"

# Default X.org log if xset q doesn't reveal it
XORG_DEFAULT_LOG="/var/log/Xorg.0.log"

# Set to yes to enable verbose
VERBOSE="yes"

# Echos the arguments if verbose
verbose()
{
	if [ "x$VERBOSE" = "xyes" ]; then
		printf "$*"
	fi
}

# Check if we run with the Software Rasterizer, this happens e.g.
# when a second screen session is opened via f-u-a on intel
check_software_rasterizer()
{
	verbose "Checking for Software Rasterizer: "
	if glxinfo 2> /dev/null | egrep -q '^OpenGL renderer string: Software Rasterizer' ; then
		verbose "present. \n";
		return 0;
	else
		verbose "Not present. \n"
		return 1;
	fi

}

# Check for non power of two texture support
check_npot_texture()
{
	verbose "Checking for non power of two support: "
	if glxinfo 2> /dev/null | egrep -q '(GL_ARB_texture_non_power_of_two|GL_NV_texture_rectangle|GL_EXT_texture_rectangle|GL_ARB_texture_rectangle)' ; then
		verbose "present. \n";
		return 0;
	else
		verbose "Not present. \n"
		return 1;
	fi

}

# Check for presence of FBConfig
check_fbconfig()
{
	verbose "Checking for FBConfig: "
	if [ "$INDIRECT" = "yes" ]; then
		$GLXINFO -i | grep -q GLX.*fbconfig 
		FB=$?
	else
		$GLXINFO | grep -q GLX.*fbconfig 
		FB=$?
	fi

	if [ $FB = "0" ]; then
		unset FB
		verbose "present. \n"
		return 0;
	else
		unset FB
		verbose "not present. \n"
		return 1;
	fi
}


# Check for TFP
check_tfp()
{
	verbose "Checking for texture_from_pixmap: "
	if [ $($GLXINFO 2>/dev/null | grep -c GLX_EXT_texture_from_pixmap) -gt 2 ] ; then
		verbose "present. \n"
		return 0;
	else
		verbose "not present. \n"
		if [ "$INDIRECT" = "yes" ]; then
			unset LIBGL_ALWAYS_INDIRECT
			INDIRECT="no"
			return 1;
		else
			verbose "Trying again with indirect rendering:\n";
			INDIRECT="yes"
			export LIBGL_ALWAYS_INDIRECT=1
			check_tfp;
			return $?
		fi
	fi
}

# Check wether the composite extension is present
check_composite()
{
	verbose "Checking for Composite extension: "
	if xdpyinfo -queryExtensions | grep -q Composite ; then
		verbose "present. \n";
		return 0;
	else
		verbose "not present. \n";
		return 1;
	fi
}

# Check if the nVidia card has enough video ram to make sense
check_nvidia_memory()
{
    if [ ! -x "$NVIDIA_SETTINGS" ]; then
	return 0
    fi

	MEM=$(${NVIDIA_SETTINGS} -q VideoRam | egrep Attribute\ \'VideoRam\'\ .*: | cut -d: -f3 | sed 's/[^0-9]//g')
	if [ $MEM -lt $NVIDIA_MEMORY ]; then
		verbose "Less than ${NVIDIA_MEMORY}kb of memory and nVidia";
		return 1;
	fi
	return 0;
}

# Check for existence if NV-GLX
check_nvidia()
{
	if [ ! -z $NVIDIA_INTERNAL_TEST ]; then
		return $NVIDIA_INTERNAL_TEST;
	fi
	verbose "Checking for nVidia: "
	if xdpyinfo | grep -q NV-GLX ; then
		verbose "present. \n"
		NVIDIA_INTERNAL_TEST=0
		return 0;
	else
		verbose "not present. \n"
		NVIDIA_INTERNAL_TEST=1
		return 1;
	fi
}

# Check if the max texture size is large enough compared to the resolution
check_texture_size()
{
	# Check how many screens we've got and iterate over them
	N=$(xdpyinfo | grep -i "number of screens" | sed 's/.*[^0-9]//g')
	for i in $(seq 1 $N); do
	    verbose "Checking screen $i"
	    TEXTURE_LIMIT=$(glxinfo -l | grep GL_MAX_TEXTURE_SIZE | sed -n "$i s/^.*=[^0-9]//g p")
	    RESOLUTION=$(xdpyinfo | grep -i dimensions: | sed -n -e "$i s/^ *dimensions: *\([0-9]*x[0-9]*\) pixels.*/\1/ p")
	    VRES=$(echo $RESOLUTION | sed 's/.*x//')
	    HRES=$(echo $RESOLUTION | sed 's/x.*//')
	    verbose "Comparing resolution ($RESOLUTION) to maximum 3D texture size ($TEXTURE_LIMIT): ";
	    if [ $VRES -gt $TEXTURE_LIMIT ] || [ $HRES -gt $TEXTURE_LIMIT ]; then
		verbose "Failed.\n"
		return 1;
	    fi
	    verbose "Passed.\n"
	done
	return 0
}

# check driver whitelist
# running_under_whitelisted_driver()
# {
# 	LOG=$(xset q|grep "Log file"|awk '{print $3}')
# 	if [ "$LOG" = "" ]; then
# 	    verbose "xset q doesn't reveal the location of the log file. Using fallback $XORG_DEFAULT_LOG \n"
# 	    LOG=$XORG_DEFAULT_LOG;
# 	fi
# 	if [ -z "$LOG" ];then
# 		verbose "AIEEEEH, no Log file found \n"
# 		verbose "$(xset q) \n"
# 	return 0
# 	fi
# 	for DRV in ${WHITELIST}; do
# 		if egrep -q "Loading ${XORG_DRIVER_PATH}${DRV}_drv\.so" $LOG &&
# 		   ! egrep -q "Unloading ${XORG_DRIVER_PATH}${DRV}_drv\.so" $LOG; 
# 		then
# 			return 0
# 		fi
# 	done
# 	verbose "No whitelisted driver found\n"
# 	return 1
# }

# check pciid blacklist
# have_blacklisted_pciid()
# {
# 	OUTPUT=$(lspci -n)
# 	for ID in ${BLACKLIST_PCIIDS}; do
# 		if echo "$OUTPUT" | egrep -q "$ID"; then
# 			verbose "Blacklisted PCIID '$ID' found \n"
# 			return 0
# 		fi
# 	done
# 	OUTPUT=$(lspci -vn | grep -i VGA)
# 	verbose "Detected PCI ID for VGA: $OUTPUT\n"
# 	return 1
# }

build_env()
{
	if check_nvidia; then
		ENV="__GL_YIELD=NOTHING "
	fi
	if [ "$INDIRECT" = "yes" ]; then
		ENV="$ENV LIBGL_ALWAYS_INDIRECT=1 "
	fi

	ENV="$ENV FROM_WRAPPER=yes"

	if [ -n "$ENV" ]; then
		export $ENV
	fi
}

build_args()
{
	if [ "x$INDIRECT" = "xyes" ]; then
		COMPIZ_OPTIONS="$COMPIZ_OPTIONS --indirect-rendering "
	fi
	if [ ! -z "$DESKTOP_AUTOSTART_ID" ]; then
		COMPIZ_OPTIONS="$COMPIZ_OPTIONS --sm-client-id $DESKTOP_AUTOSTART_ID"
	fi
	if check_nvidia; then
		if [ "x$INDIRECT" != "xyes" ]; then
			COMPIZ_OPTIONS="$COMPIZ_OPTIONS --loose-binding"
		fi
	fi
}

####################
# Execution begins here.

if [ "x$LIBGL_ALWAYS_INDIRECT" = "x1" ]; then
	INDIRECT="yes";
fi

# if we run under Xgl, we can skip some tests here

# if vesa or vga are in use, do not even try glxinfo (LP#119341)
#if ! running_under_whitelisted_driver || have_blacklisted_pciid; then
#    exit 1;
#fi
# check if we have the required bits to run compiz and if not, 
# fallback
if ! check_tfp || ! check_npot_texture || ! check_composite || ! check_texture_size; then
    exit 1;
fi

# check if we run with software rasterizer and if so, bail out
if check_software_rasterizer; then
    verbose "Software rasterizer detected, aborting"
    exit 1;
fi

if check_nvidia && ! check_nvidia_memory; then
    exit 1;
fi

if ! check_fbconfig; then
    exit 1;
fi


# get environment
build_env
build_args

if [ "x$CM_DRY" = "xyes" ]; then
	verbose "Dry run finished: everything should work with regards to Compiz and 3D.\n"
	verbose "Execute: ${COMPIZ_NAME} $COMPIZ_OPTIONS "$@" $COMPIZ_PLUGINS \n"
	exit 0;
fi

${COMPIZ_NAME} $COMPIZ_OPTIONS "$@" $COMPIZ_PLUGINS

