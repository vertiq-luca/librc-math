prefix			:= /usr
RM				:= rm -r -f
INSTALL			:= install -m 755
INSTALLDIR		:= install -d -m 755
INSTALLDIRWRITE	:= install -d -m 777
INSTALLNONEXEC	:= install -m 644

all:
	@make -C library --no-print-directory
	@make -C examples --no-print-directory

install:
	@$(INSTALLDIR) $(DESTDIR)$(prefix)/bin
	@make -C library -s install
	@make -C examples -s install

clean:
	@make -C library -s clean
	@make -C examples -s clean
	@echo "All Directories Cleaned"


uninstall:
	@make -C library -s uninstall
	@make -C examples -s uninstall
	@echo "Uninstall Complete"
