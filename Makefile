CFLAGS = -g3 -O2 -Wall -Wunused -Werror `pkg-config --cflags libnotify`
LDFLAGS = -lstrophe -lssl -lresolv -lexpat `pkg-config --libs libnotify`

TARGET = j
SRC = src/main.c src/xmpp.c src/libnotify.c

all: $(TARGET)

$(TARGET): $(SRC)
	gcc $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
