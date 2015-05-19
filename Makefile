#------------------------------------------------------------------------------

SOURCE=EMonCMS.cpp LinuxTests.cpp EMonCMS.h Debug.h
MYPROGRAM=emoncmstest

CC=g++

#------------------------------------------------------------------------------



all: $(MYPROGRAM)



$(MYPROGRAM): $(SOURCE)

	$(CC) $(SOURCE) -DLINUX -o$(MYPROGRAM)

clean:

	rm -f $(MYPROGRAM)
