# I moduli usano libce, quindi dobbiamo sapere dove si trovano libce.h e
# libce64.a. La libreria installa un file libce.conf al cui interno sono
# definite alcune variabili, tra cui: CE_INCLUDE (path della directory che
# contiene libce.h) e CE_LIB64 (path della directory che contiene libce64.a).
#
# Il percorso di libce.conf può essere passato tramite la variabile di ambiente
# LIBCECONF. Se questa manca, proviamo a leggerlo dallo script compile, se
# è raggiungibile dal PATH della shell.
CE_COMPILE := $(shell which compile)
ifeq ($(LIBCECONF),)
ifeq ($(CE_COMPILE),)
$(error "LIBCECONF non settata e compile non trovato")
endif
LIBCECONF := $(lastword $(shell grep ^source $(CE_COMPILE)))
endif
include $(LIBCECONF)

# START_IO e START_UTENTE dipendono dalla suddivisione della memoria virtuale
# definita in costanti.h. Invece di calcolarli a priori, compiliamo un piccolo
# programma (util/mkstart.cpp) che include costanti.h e scrive la definizione
# delle due variabili in conf/start.mk, che includiamo qui.
-include conf/start.mk

# alcuni esercizi definiscono ulteriori variabili di compilazione
-include conf/conf.mk

# Opzioni per il compilatore:
#
# Il modulo sistema viene compilato con compile, con le stesse opzioni
# di tutti gli altri programmi bare. Qui raccogliamo le opzioni per
# la compilazione del modulo I/O e utente.
#
# * -Wall:		massimo livello di warning
# * -Iinclude:		cerca file .h nella directory 'include'
# * -I$(CE_INCLUDE):	path della directory che contiene libce.h
#
# * disabilitiamo alcune funzionalità del C++ che richiedono un supporto
#   runtime che non abbiamo implementato:
# 	* -fno-exceptions:	eccezioni
# 	* -fno-rtti:		Run Time Type Information
#
# * disabilitiamo alcune protezioni del codice che o richiedono un supporto
#   runtime, o rendono il codice assembler più confuso
#   	* -fno-stack-protector:	stack canaries
#   	* -fcf-protection=none:	control-flow protection
#
# * disabilitiamo alcune ottimizzazioni che rendono il codice più difficile
#   da debuggare, oppure non possono essere usate in codice di livello sistema
#   	* -fno-omit-frame-pointer:	usa rbp per contenere il frame pointer
#   	* -fno-inline			non espandere inline le funzioni
#   	* -mno-sse:			non usare le istruzioni SSE
#
# * -Og:		ottimizza per il debugging
#
# * -ggdb3:		includi le informazioni per il debugger
#
# * -std=c++17		usa lo standard C++17 [per new(val_align_t)]
#
# * -ffreestanding	non assumere che esista la libreria standard
#
# La variabile di ambiente CFLAGS può essere usata per passare ulteriori argomenti.
NCFLAGS=			\
	-Wall 			\
	-Iinclude		\
	-I$(CE_INCLUDE)		\
	-fno-exceptions 	\
	-fno-rtti 		\
	-fno-stack-protector 	\
	-fcf-protection=none	\
	-fno-omit-frame-pointer \
	-fno-inline		\
	-mno-sse		\
	-Og			\
	-ggdb3			\
	-std=c++17		\
	-ffreestanding		\
	$(CFLAGS)

# se è definita la variabile di ambiente AUTOCORR definiamo la macro AUTOCORR,
# che permette di eseguire il codice in maniera non interattiva. In
# particolare, la funzione pause() viene trasformata in una NOP e i messaggi
# scritti sul video vengono inviati sul log, con livello USR.
ifdef AUTOCORR
	NCFLAGS+=-DAUTOCORR
endif

# Opzioni per il collegatore:
#
# * -nostdlib:		non collegare le librerie standard
# * --no-relax:		disabilita alcune ottimizzazioni che non sono compatibili con
# 			il nostro modello
# * -z noexecstack	non ci riguarda, ma va passato per evitare un warning
# * -z norelro		disabilita una protezione che non ci riguarda
# * -L$(CE_LIB64):	path della directory che contiene libce64.a
#
# La variabile di ambiente LDFLAGS può essere usata per passare ulteriori argomenti.
NLDFLAGS=			\
        -nostdlib		\
	--no-relax		\
	-z noexecstack		\
	-z norelro		\
	-L$(CE_LIB64)		\
	$(LDFLAGS)

# Librerie da collegare con i vari moduli:
#
# * - lce64:		libce64.a
#
# La variabile di ambiente LDLIBS può essere usata per collegare ulteriori librerie.
NLDLIBS=			\
	-lce64			\
	$(LDLIBS)

# target di default del Makefile: versioni boot e debug dei tre moduli
all: 				\
     boot/0-sistema		\
     boot/1-io			\
     boot/2-utente		\
     debug/0-sistema		\
     debug/1-io			\
     debug/2-utente

##############################################################################
# regole per la costruzione dei tre moduli: sistema, I/O e utente            #
##############################################################################

# Il modulo sistema si ottiene da sistema.cpp, sistema.s e libce
# Il modulo sistema è un normale programma 'bare', quindi si può usare lo script compile.
debug/0-sistema boot/0-sistema &: sistema/sistema.cpp sistema/sistema.s include/*
	@$(CE_COMPILE) sistema

# Il modulo I/O si ottiene da io.cpp, io.s e libce
#
# Usiamo lo stesso linker script dello script compile ($CE_LIB32/boo.ld), ma
# impostando il simbolo LOAD_ADDRESS con l'indirizzo di partenza della 
# parte IO/condivisa ($START_IO, definita in conf/start.mk)
debug/1-io: io/io_s.o io/io_cpp.o
	@echo "   [make] ==> Collego  $@      [$(START_IO)]"
	@eval $(CE_LD64)				\
		$(NLDFLAGS)				\
		-o $@					\
		--defsym=LOAD_ADDRESS=$(START_IO)	\
		-T$(CE_LIB32)/boot.ld			\
		io/io_s.o				\
		io/io_cpp.o				\
		$(NLDLIBS)

# il modulo utente si ottiene da utente.cpp, utente.s, utente/lib.cpp e libce
#
# Usiamo lo stesso linker script dello script compile ($CE_LIB32/boo.ld), ma
# impostando il simbolo LOAD_ADDRESS con l'indirizzo di partenza della 
# parte utente/condivisa ($START_UTENTE, definita in conf/start.mk)
debug/2-utente: utente/uten_s.o utente/lib.o utente/uten_cpp.o
	@echo "   [make] ==> Collego  $@  [$(START_UTENTE)]"
	@eval $(CE_LD64)				\
		$(NLDFLAGS)				\
		-o $@					\
		--defsym=LOAD_ADDRESS=$(START_UTENTE)	\
		-T$(CE_LIB32)/boot.ld			\
		utente/uten_cpp.o			\
		utente/uten_s.o				\
		utente/lib.o				\
		$(NLDLIBS)

# Il boot loader carica in memoria una versione "stripped" dei moduli (senza le
# informazioni di debug), per risparmiare memoria. Questo non crea problemi perché
# il debugger usa la versione completa dei moduli che si trova nella macchina
# ospite, non la copia caricata nella macchina virtuale.
boot/%: debug/%
	@echo "   [make] ==> Genero   $@"
	@eval $(CE_STRIP) -s $^ -o $@

# assemblamento di io.s
io/io_s.o: io/io.s include/costanti.h
	@echo "   [make] ==> Assemblo io/io.s"
	@eval $(CE_CXX64)		\
		$(NCFLAGS)		\
		-x assembler-with-cpp	\
		-c			\
		io/io.s			\
		-o io/io_s.o

# compilazione di io.cpp.
# Siccome il modulo I/O girerà a livello sistema con le interruzioni abilitate,
# dobbiamo disabilitare la red-zone (-mno-red-zone).
io/io_cpp.o: io/io.cpp include/costanti.h include/sysio.h include/sys.h include/io.h
	@echo "   [make] ==> Compilo  io/io.cpp"
	@eval $(CE_CXX64)		\
		$(NCFLAGS)		\
		-mno-red-zone		\
		-fpic			\
		-c			\
		io/io.cpp		\
		-o io/io_cpp.o

# assemblamento di utente.s
utente/uten_s.o: utente/utente.s include/costanti.h
	@echo "   [make] ==> Assemblo utente/utente.s"
	@eval $(CE_CXX64)		\
		$(NCFLAGS)		\
		-x assembler-with-cpp	\
		-c			\
		utente/utente.s		\
		-o utente/uten_s.o

# compilazione di utente.cpp
utente/uten_cpp.o: utente/utente.cpp include/costanti.h include/sys.h include/io.h utente/lib.h
	@echo "   [make] ==> Compilo  utente/utente.cpp"
	@eval $(CE_CXX64)		\
		$(NCFLAGS)		\
		-fpic			\
		-Iutente		\
		-c			\
		utente/utente.cpp	\
		-o utente/uten_cpp.o

# compilazione di lib.cpp
utente/lib.o: utente/lib.cpp include/costanti.h include/sys.h include/io.h utente/lib.h
	@echo "   [make] ==> Compilo  utente/lib.cpp"
	@eval $(CE_CXX64)		\
		$(NCFLAGS)		\
		-fpic			\
		-Iutente		\
		-c			\
		utente/lib.cpp		\
		-o utente/lib.o

# File di supporto.
#
# Alcuni valori scritti in costanti.h servono anche a make e agli script boot e
# debug. Il programma mkstart legge questi valori e li scrive in dei file che
# questi altri strumenti sono in grado di interpretare.
util/mkstart: include/costanti.h util/mkstart.cpp
	@g++						\
		-std=c++17				\
		-fno-exceptions				\
		-g					\
		-Iinclude				\
		-I$(CE_INCLUDE)				\
		-DCE_UTILS				\
		-DCE_ADDR2LINE='"$(CE_ADDR2LINE)"'	\
		-o util/mkstart util/mkstart.cpp

# - start.mk serve a make ed è incluso all'inizio di questo stesso Makefile
# - conf.sh serve allo script boot
# - const.gdb serve allog script debug
conf/start.mk conf/conf.sh conf/const.gdb &: util/mkstart
	@util/mkstart

# pulizia
.PHONY: clean reset doc
clean:
	@echo "   [make] ==> Elimino tutti i file oggetto"
	@rm -f sistema/*.o io/*.o utente/*.o util/*.o
	@rm -f util/mkstart util/start.mk util/start.pl util/tmp.gdb

reset: clean
	@echo "   [make] ==> Elimino tutti i moduli"
	@rm -rf boot/* doc/html doc/latex

# Creazione della documentazione.
#
# Il file tags è usato (tra gli altri) da vim.
tags: $(wildcard */*.cpp */*.h)
	@ctags				\
		--recurse=yes		\
		--exclude=build/	\
		--exclude=.git		\
		--exclude=\*.o		\
		--exclude=\*.patch	\
		--exclude=\*.rej	\
		--exclude=\*.orig	\
		--exclude=Makefile	\
		--exclude=run		\
		--exclude=\*.mk		\
		--exclude=\*.py		\
		--exclude=\*.pl		\
		--exclude=\*.gdb	\
		.			\
		$(CE_INCLUDE)		\
		$(if $(CE_SOURCES),$(addprefix $(CE_SOURCES)/,*/*.cpp include/* as64/*.s))

# se è definita la variabile CE_ESAME, si tratta di un nucleo modificato per un
# testo di esame. In questo caso CE_ESAME contiene la data dell'appello, e il
# file util/question.tex contiene la traccia.
ifeq ($(CE_ESAME),)
doc: doc/html/index.html
	@true

doc/html/index.html: $(wildcard */*.cpp */*.s */*.h)
	@PROJECT_NUMBER=$$(git describe --tags HEAD) doxygen
else
doc: doc/html/index.html
	@true

doc/html/index.html: question.dox $(wildcard */*.cpp */*.s */*.h)
	@PROJECT_NUMBER=$(CE_ESAME) doxygen
	@sed -i -e 's|\([a-z>")]\)\.\([A-Z<]\)|\1\.<p>\2|g;s|$$\([^$$]*\)\$$|<i>\1</i>|g;s|\\math[a-z]*{\([^}]*\)}|\1|g' doc/html/group__esame.html

# versione doxygen della traccia, a partire dal markdown
question.dox: util/question.md
	@( echo '/// @ingroup esame';					\
	  d=$$(export LANG=it_IT.utf8; date +'%-e %B %Y' -d $(CE_ESAME) 2>/dev/null || date -j -f '%Y-%m-%d' $(CE_ESAME) +'%e %B %Y');	\
	  case "$$d" in							\
	  1\ *|8*|11*) h="dell'$$d";;					\
	  *)     h="del $$d";;						\
	  esac;								\
	  echo "/// @page esame Prova pratica $$h"; sed 's|^|/// |' $^) > question.dox

# versione markdown della traccia, a partire dal LaTeX
util/question.md: util/question.tex
	@pandoc -o util/question.md util/question.tex
endif
