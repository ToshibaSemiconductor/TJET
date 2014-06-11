#=====================================================================
# Rules
#=====================================================================
.PHONY: all clean

all:
	$(MAKE) -C sdioapi;
	$(MAKE) -C tjet-driver;
	@cp -rf tjet-driver/objs ./;
	@cp -rf sdioapi/objs ./;

clean:
	$(MAKE) clean -C tjet-driver;
	$(MAKE) clean -C sdioapi;
	@rm -rf objs

# DO NOT DELETE
