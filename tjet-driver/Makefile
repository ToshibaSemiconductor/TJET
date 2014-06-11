#*********************************************************************
##  Title   : Makefile
##
##  Descript: Makefile
#*********************************************************************


#=====================================================================
# Directries
#=====================================================================
export JET_TOP_DIR = ${shell pwd}
export JET_SRC_DIR = $(JET_TOP_DIR)/src
export JET_OBJ_DIR = $(JET_TOP_DIR)/objs




#=====================================================================
# Files
#=====================================================================
ifeq ($(strip X$(JET_CONFIG_MAKE)),X)
	JET_CONFIG_MAKE := Config.make
endif

include $(JET_CONFIG_MAKE)


JET_SRC_SUB_DIRS +=	$(JET_CNL__BLD_DIR)




#=====================================================================
# C flags
#=====================================================================
export JET_INCLUDES = -I. -I$(JET_SRC_DIR)/include \
                      $(patsubst %,-I$(JET_SRC_DIR)/%,$(JET_INCLUDE_DIRS)) 
export JET_CFLAGS   = $(JET_INCLUDES) $(JET_DEFINES) $(JET_DEBUG)




#=====================================================================
# AR flags
#=====================================================================




#=====================================================================
# Linker flags
#=====================================================================




#=====================================================================
# Tools
#=====================================================================




#=====================================================================
# Rules
#=====================================================================
.PHONY: all module clean trashclean realclean




all:
	@echo $(JET_INCLUDE_DIRS);
	@echo $(JET_INCLUDES);
	@for x in $(JET_SRC_SUB_DIRS) ; do       \
		cd $(JET_SRC_DIR)/$$x || exit 1;     \
		$(MAKE) $@ || exit 1;                \
		cp tos*.ko $(JET_OBJ_DIR) || exit 1; \
	done


clean:
	@for x in $(JET_SRC_SUB_DIRS) ; do       \
		cd $(JET_SRC_DIR)/$$x || exit 1;     \
		$(MAKE) $@ || exit 1;                \
	done
	@rm $(JET_OBJ_DIR)/tos*.ko


trashclean:
	find  . \( -name "*.o" \
	        -o -name "*~"  \
	        -o -name core -type f \) \
	        -exec rm {} \;


realclean: trashclean clean


# DO NOT DELETE
