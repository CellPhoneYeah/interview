all:
# ifeq ($(debug), 1)
	cd ./3rd && make
	cd ./net && make debug=1
$(info "compile finished.")
# else
# 	cd ./net && make clean && make
# endif

.PHONY : clean
clean:
	cd ./3rd && make clean
	cd ./net && make clean