all:
ifeq ($(debug), 1)
	cd ./net && make clean && make debug=1
else
	cd ./net && make clean && make
endif
$(info "compile finished.")