#*********************************************************************
##  Title   : Config-Ref.make
##
##  Descript: configuration files
#*********************************************************************

export


#=====================================================================
# Supported Driver Object
#=====================================================================
JET_FIT__DRV_NAME = toscnlfit
JET_IOFT_DRV_NAME = tosiofit




#=====================================================================
# Configuration of build options
#=====================================================================

# Compiler Configuration
#---------------------------------------------------------------------

# general compile option #


# build and include path #

ifneq ($(strip X$(JET_FIT__DRV_NAME)), X)
	JET_FIT__BLD_DIR = cnl/fitting
	JET_FIT__INC_DIR = cnl/fitting
endif

ifneq ($(strip X$(JET_IOFT_DRV_NAME)), X)
	JET_IOFT_BLD_DIR = io/fitting
	JET_IO___INC_DIR = io/include
endif


JET_SRC_SUB_DIRS += $(JET_FIT__BLD_DIR) \
					$(JET_IOFT_BLD_DIR)

JET_INCLUDE_DIRS += $(JET_FIT__INC_DIR) \
					$(JET_IO___INC_DIR)


