# directories
SRCDIR		:= src
LIBDIR		:= lib
BUILDDIR	:= build
INCLUDEDIR	:= include
SHORTNAME	:= librc_math.so
SONAME		:= librc_math.so.1
FULLNAME	:= librc_math.so.1.0.0
TARGET		:= $(LIBDIR)/$(FULLNAME)

# file definitions for rules
SOURCES		:= $(shell find $(SRCDIR) -type f -name *.c)
OBJECTS		:= $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
INCLUDES	:= $(shell find $(INCLUDEDIR) -name '*.h')

# compiler and linker binaries
CC			:= gcc
LINKER		:= gcc

# general compiler flags
WFLAGS		:= -Wall -Wextra -Werror=float-equal -Wuninitialized \
				-Wunused-variable -Wdouble-promotion -pedantic \
				-Wmissing-prototypes -Wmissing-declarations -Werror=undef
CFLAGS		:= -std=c99 -g -fPIC -I $(INCLUDEDIR)
OPT_FLAGS	:= -O3 -ffast-math -ftree-vectorize
LDFLAGS		:= -lm -shared -Wl,-soname,$(SONAME)

# commands
RM			:= rm -rf
INSTALL		:= install -m 755
INSTALLDIR	:= install -d -m 755

# prefix variable for making debian package
prefix		?= /usr



# linking rules
# symlink only for local examples compilation, not for install
$(TARGET): $(OBJECTS)
	@mkdir -p $(LIBDIR)
	@$(LINKER) -o $(TARGET) $(OBJECTS) $(LDFLAGS)
	@ln -sf $(FULLNAME) $(LIBDIR)/$(SONAME)
	@echo "Done making $(TARGET)"

# rule for objects
$(BUILDDIR)/%.o : $(SRCDIR)/%.c $(INCLUDES)
	@mkdir -p $(dir $(@))
	@$(CC) -c $(CFLAGS) $(WFLAGS) $(OPT_FLAGS) $(DEBUGFLAG) $< -o $(@)
	@echo "made: $(@)"




# rule for target
all:	$(TARGET)

# compile with debug symbols and DEBUG defined
debug:
	$(MAKE) $(MAKEFILE) DEBUGFLAG="-g -D DEBUG"
	@echo "$(TARGET) Make Debug Complete"


install:
	$(MAKE)
	@$(INSTALLDIR) $(DESTDIR)$(prefix)/include
	@cp -r include/* $(DESTDIR)$(prefix)/include
	@$(INSTALLDIR) $(DESTDIR)$(prefix)/lib
	@$(INSTALL) $(TARGET) $(DESTDIR)$(prefix)/lib
	@ln -sf $(FULLNAME) $(DESTDIR)$(prefix)/lib/$(SONAME)
	@ln -sf $(FULLNAME) $(DESTDIR)$(prefix)/lib/$(SHORTNAME)
	@echo "Library Install Complete"

# cleanup local binaries
clean:
	@$(RM) $(LIBDIR)
	@$(RM) $(BUILDDIR)
	@echo "Library Clean Complete"

# uninstall completely from system
uninstall:
	$(RM) $(DESTDIR)$(prefix)/lib/$(FULLNAME)
	$(RM) $(DESTDIR)$(prefix)/lib/$(SONAME)
	$(RM) $(DESTDIR)$(prefix)/lib/$(SHORTNAME)
	$(RM) $(DESTDIR)$(prefix)/include/rc_math.h
	$(RM) $(DESTDIR)$(prefix)/include/rc_math
	@echo "Library Uninstall Complete"




