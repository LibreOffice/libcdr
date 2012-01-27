#!/bin/sh

# Script to create the BUILDNUMBER used by compile-resource. This script
# needs the script createBuildNumber.pl to be in the same directory.

{ perl ./createBuildNumber.pl \
	src/lib/libcdr-build.stamp \
	src/conv/raw/cdr2raw-build.stamp \
	src/conv/svg/cdr2xhtml-build.stamp
#Success
exit 0
}
#unsucessful
exit 1
