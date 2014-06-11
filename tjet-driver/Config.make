#*********************************************************************
##  Title   : Config.make
##
##  Descript: configuration files
#*********************************************************************

export
export KERNELDIR ?= 

ARCH=arm
CROSS_COMPILE=arm-eabi-

#=====================================================================
# Configuration Switches
#=====================================================================
#
# Debug Switches.
# PROD for production (no debugging/most optimized code) - Default.
# TEST for testing (R&D with debugging)
# COVR for converage test
#
OBJ_STATE = OBJ_PROD
#OBJ_STATE = OBJ_TEST
#OBJ_STATE = OBJ_COVR


#
# CPU section.
#
USE_CPU_X86    =        # use X86 platform
USE_CPU_ARM    = y      # use ARM platform
USE_CPU_MIPS   =        # use MIPS platform


#
# OS section
#
USE_OS_LINUX   = y


#
# debug level
# this value is enable only if STATE = TEST
#
DEBUG_LVL      = 0


#
# RF setting regist num max
#
RFARRAYNUM = 20

#=====================================================================
# Supported Driver Object
#=====================================================================
JET_CMOS_DRV_NAME = tososcmn
JET_BUS__DRV_NAME = tosbuscmn
JET_CNL__DRV_NAME = toscnl




#=====================================================================
# Definitions as build option
#=====================================================================

# CPU section
#---------------------------------------------------------------------
ifneq ($(strip X$(USE_CPU_X86)), X)
endif

ifneq ($(strip X$(USE_CPU_ARM)), X)
endif


# OS section
#---------------------------------------------------------------------
ifneq ($(strip X$(USE_OS_LINUX)),     X)
	JET_DEFINES += -DUSE_OS_LINUX
endif




#=====================================================================
# Configuration of build options
#=====================================================================

# Compiler and Linker selection
#---------------------------------------------------------------------

ifneq ($(strip X$(USE_CPU_X86)), X)
	JET_DEFINES += -DUSE_LE_CPU
endif

ifneq ($(strip X$(USE_CPU_ARM)), X)
	JET_DEFINES += -DUSE_LE_CPU
endif

ifneq ($(strip X$(USE_CPU_MIPS)), X)
	JET_DEFINES += -DUSE_BE_CPU
endif


# Compiler Configuration
#---------------------------------------------------------------------

# general compile option #


# debug/optimize option #

ifeq ($(strip X$(OBJ_STATE)),XOBJ_TEST)
	JET_DEBUG   += -DDEBUG_LVL=$(DEBUG_LVL)
endif

ifeq ($(strip X$(OBJ_STATE)),XOBJ_PROD)
	JET_DEBUG   += -DDEBUG_LVL=0
endif


# RF setting regist num max

ifeq ($(strip X$(RFARRAYNUM)), X0)
	RFARRAYNUM=1
endif

ifneq ($(strip X$(RFARRAYNUM)), X)
	JET_DEBUG += -DDBG_ARRAYNUM=$(RFARRAYNUM)
else
	JET_DEBUG += -DDBG_ARRAYNUM=1
endif

# build and include path #

ifneq ($(strip X$(JET_CMOS_DRV_NAME)), X)
	JET_CMOS_BLD_DIR = os/linux/syscall
	JET_CMOS_INC_DIR = os/linux/include
endif

ifneq ($(strip X$(JET_BUS__DRV_NAME)), X)
	JET_BUS__BLD_DIR = bus
	JET_BUS__INC_DIR = bus/include
endif

ifneq ($(strip X$(JET_CNL__DRV_NAME)), X)
	JET_CNL__BLD_DIR = cnl/core/izan
	JET_CNL__INC_DIR = cnl/core/include cnl/core/izan
endif


JET_SRC_SUB_DIRS += $(JET_CMOS_BLD_DIR) \
					$(JET_BUS__BLD_DIR)


JET_INCLUDE_DIRS += $(JET_CMOS_INC_DIR) \
					$(JET_BUS__INC_DIR) \
					$(JET_CNL__INC_DIR)




#=====================================================================
# Other options
#=====================================================================

include Config-Ref.make


