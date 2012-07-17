D = ..
include $(D)/Makefile.in

all: create_bin_folder
	cd test; make; cd ..;
	cd lib; make; cd ..;
	cd bin; make; cd ..;

create_bin_folder:
	mkdir -p $(INITRD_BIN_FOLDER)

clean:
	cd test; make clean; cd ..;
	cd lib; make clean; cd ..;
	cd bin; make clean; cd ..;
	rm -rf $(INITRD_BIN_FOLDER)
