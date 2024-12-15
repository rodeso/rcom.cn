CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = download
SRC = protocol.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

pipe:
	rm -f $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -DPIPE
	./$(TARGET) ftp://rcom:rcom@ftp.netlab.fe.up.pt/pipe.txt

crab:
	rm -f $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -DCRAB
	./$(TARGET) ftp://rcom:rcom@ftp.netlab.fe.up.pt/files/crab.mp4

lr:
	rm -f $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -DLR
	./$(TARGET) ftp://rcom:rcom@ftp.netlab.fe.up.pt/debian/ls-lR.gz

parrot:
	rm -f $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -DPARROT
	./$(TARGET) ftp://rcom:rcom@ftp.netlab.fe.up.pt/pub/parrot/iso/testing/Parrot-architect-5.3_amd64.iso

welcome:
	rm -f $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -DWELCOME
	./$(TARGET) ftp://rcom:rcom@ftp.netlab.fe.up.pt/welcome.msg

ubuntu:
	rm -f $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -DUBUNTU
	./$(TARGET) ftp://rcom:rcom@ftp.netlab.fe.up.pt/ubuntu.iso

elisp:
	rm -f $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -DELISP
	./$(TARGET) ftp://ftp.up.pt/pub/gnu/emacs/elisp-manual-21-2.8.tar.gz

speedtest:
	rm -f $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -DSPEEDTEST
	./$(TARGET) ftp://anonymous:anonymous@ftp.bit.nl/speedtest/100mb.bin

rebex:
	rm -f $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -DREBEX
	./$(TARGET) ftp://demo:password@test.rebex.net/readme.txt

.PHONY: all clean pipe crab lr parrot welcome ubuntu elisp speedtest rebex