#
# Generated - do not edit!
#
# NOCDDL
#
CND_BASEDIR=`pwd`
CND_BUILDDIR=build
CND_DISTDIR=dist
# default configuration
CND_PLATFORM_default=AVR-Windows
CND_ARTIFACT_DIR_default=dist/default/production
CND_ARTIFACT_NAME_default=Final_Project.X.production.hex
CND_ARTIFACT_PATH_default=dist/default/production/Final_Project.X.production.hex
CND_PACKAGE_DIR_default=dist/default/package
CND_PACKAGE_NAME_default=finalproject.x.tar
CND_PACKAGE_PATH_default=dist/default/package/finalproject.x.tar
#
# include compiler specific variables
#
# dmake command
ROOT:sh = test -f nbproject/private/Makefile-variables.mk || \
	(mkdir -p nbproject/private && touch nbproject/private/Makefile-variables.mk)
#
# gmake command
.PHONY: $(shell test -f nbproject/private/Makefile-variables.mk || (mkdir -p nbproject/private && touch nbproject/private/Makefile-variables.mk))
#
include nbproject/private/Makefile-variables.mk

# Environment 
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib


# build
build: .build-post

.build-pre:
# Add your pre 'build' code here...

.build-post: .build-impl
# Add your post 'build' code here...


# clean
clean: .clean-post

.clean-pre:
# Add your pre 'clean' code here...

.clean-post: .clean-impl
# Add your post 'clean' code here...


# clobber
clobber: .clobber-post

.clobber-pre:
# Add your pre 'clobber' code here...

.clobber-post: .clobber-impl
# Add your post 'clobber' code here...


# all
all: .all-post

.all-pre:
# Add your pre 'all' code here...

.all-post: .all-impl
# Add your post 'all' code here...


# help
help: .help-post

.help-pre:
# Add your pre 'help' code here...

.help-post: .help-impl
# Add your post 'help' code here...



# include project implementation makefile
include nbproject/Makefile-impl.mk

# include project make variables
include nbproject/Makefile-variables.mk
