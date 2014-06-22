# TOP_DIR and OBJ_DIR should be set by the user to the right directories,
# if necessary.

TOP_DIR ?= ../../..
OBJ_DIR ?= $(TOP_DIR)/objs


# The setup below is for gcc on a Unix-like platform.

SRC_DIR = $(TOP_DIR)/src/tools/ftrandom

CC = gcc
WFLAGS = -Wmissing-prototypes \
         -Wunused \
         -Wimplicit \
         -Wreturn-type \
         -Wparentheses \
         -pedantic \
         -Wformat \
         -Wchar-subscripts \
         -Wsequence-point
CFLAGS = $(WFLAGS) \
         -g \
         -I $(TOP_DIR)/include
LIBS = -lm \
       -L $(OBJ_DIR) \
       -lfreetype \
       -lz

all: $(OBJ_DIR)/ftrandom

$(OBJ_DIR)/ftrandom: $(SRC_DIR)/ftrandom.c $(OBJ_DIR)/libfreetype.a
	$(CC) -o $(OBJ_DIR)/ftrandom $(CFLAGS) $(SRC_DIR)/ftrandom.c $(LIBS)

# EOF
