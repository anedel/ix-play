## SPDX-License-Identifier: GPL-2.0-or-later
##
## Copyright (c) 2014--2025 Alexandru Nedel
##
## (add your name here when you make significant changes to this file,
##  if you want to)
##
## --------------------------------------------------


.SUFFIXES: ## Don't use Old-Fashioned Suffix Rules


OBJDIR = _obj


CC = gcc
LIBS =
INCLUDES = -I../../play-utils -I..
CFLAGS = -D_POSIX_C_SOURCE=200809L \
  -g \
  -fstrict-aliasing -Wstrict-aliasing=3 -Wall -Wextra
## -DNDEBUG


vpath %    ../../play-utils
vpath %    ..


## List with all headers = a safe default for
## the header dependencies for every compilation unit.
## Accurate dependencies would be better, but not worth the bother:
## rebuilding everything specified in this Makefile takes 2.5 seconds.
##
DEPS = \
  util-input.h \
  util-mutexattr.h \
  util-ofd-flags.h \
  util-sigaction.h \
  util-timespec.h \
  util-timeval.h \
  loop-errno-sig.h \
  loop-handling-sig.h \
  util-ex-threads.h


_LOOP_ERRNO_SIG_SRCS = \
  util-sigaction.c \
  loop-errno-sig.c

LOOP_ERRNO_SIG_OBJS = $(addprefix $(OBJDIR)/,$(subst .c,.o,$(_LOOP_ERRNO_SIG_SRCS)))


_LOOP_HANDLING_SIG_SRCS = \
  util-sigaction.c \
  util-timespec.c \
  util-timeval.c \
  loop-handling-sig.c

LOOP_HANDLING_SIG_OBJS = $(addprefix $(OBJDIR)/,$(subst .c,.o,$(_LOOP_HANDLING_SIG_SRCS)))


_COMMON_SRCS = \
  


COMMON_OBJS = $(addprefix $(OBJDIR)/,$(subst .c,.o,$(_COMMON_SRCS)))


EXENAMES = \
  za-empty \
  za-true1 \
  za-true2 \
  za-true3 \
  za-true4 \
  za-true5 \
  za-printf1 \
  za-printf2 \
  za-printf3 \
  za-printf4 \
  za-printf5 \
  za-printf6 \
  za-exec1 \
  za-exec2 \
  za-exec3 \
  za-exec4 \
  za-loop-dup \
  za-loop-errno-sig \
  za-loop-errno-sig-lpthread \
  za-loop-read \
  za-mmap-split-merge \
  za-rtsig-send \
  za-rtsig-handle-async \
  za-rtsig-wait-sync \
  za-pthread-lifecycle \
  za-pthread-cancel \
  za-pthreads-condvar-sem \
  za-pthreads-dup \
  za-pthreads-loop-errno-sig \
  za-pthreads-sig


.PHONY: all
all: $(EXENAMES)


za-empty: $(OBJDIR)/za-empty.o
	$(CC) -o $@ $^ $(CFLAGS)

za-true1: $(OBJDIR)/za-true1.o
	$(CC) -o $@ $^ $(CFLAGS)

za-true2: $(OBJDIR)/za-true2.o
	$(CC) -o $@ $^ $(CFLAGS)

za-true3: $(OBJDIR)/za-true3.o
	$(CC) -o $@ $^ $(CFLAGS)

za-true4: $(OBJDIR)/za-true4.o
	$(CC) -o $@ $^ $(CFLAGS)

za-true5: $(OBJDIR)/za-true5.o
	$(CC) -o $@ $^ $(CFLAGS)

za-printf1: $(OBJDIR)/za-printf1.o
	$(CC) -o $@ $^ $(CFLAGS)

za-printf2: $(OBJDIR)/za-printf2.o
	$(CC) -o $@ $^ $(CFLAGS)

za-printf3: $(OBJDIR)/za-printf3.o
	$(CC) -o $@ $^ $(CFLAGS)

za-printf4: $(OBJDIR)/za-printf4.o
	$(CC) -o $@ $^ $(CFLAGS)

za-printf5: $(OBJDIR)/za-printf5.o
	$(CC) -o $@ $^ $(CFLAGS)

za-printf6: $(OBJDIR)/za-printf6.o
	$(CC) -o $@ $^ $(CFLAGS)

za-exec1: $(OBJDIR)/za-exec1.o
	$(CC) -o $@ $^ $(CFLAGS)

za-exec2: $(OBJDIR)/za-exec2.o
	$(CC) -o $@ $^ $(CFLAGS)

za-exec3: $(OBJDIR)/za-exec3.o
	$(CC) -o $@ $^ $(CFLAGS)

za-exec4: $(OBJDIR)/za-exec4.o
	$(CC) -o $@ $^ $(CFLAGS)

za-loop-dup: $(OBJDIR)/za-loop-dup.o
	$(CC) -o $@ $^ $(CFLAGS)

za-loop-errno-sig: $(OBJDIR)/za-loop-errno-sig.o $(LOOP_ERRNO_SIG_OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

za-loop-errno-sig-lpthread: $(OBJDIR)/za-loop-errno-sig.o $(LOOP_ERRNO_SIG_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) -lpthread

za-loop-read: $(OBJDIR)/za-loop-read.o $(OBJDIR)/util-ofd-flags.o $(OBJDIR)/util-timeval.o
	$(CC) -o $@ $^ $(CFLAGS) -lm

za-mmap-split-merge: $(OBJDIR)/za-mmap-split-merge.o $(OBJDIR)/util-input.o
	$(CC) -o $@ $^ $(CFLAGS)

za-rtsig-send: $(OBJDIR)/za-rtsig-send.o $(OBJDIR)/util-timeval.o
	$(CC) -o $@ $^ $(CFLAGS) -lm

za-rtsig-handle-async: $(OBJDIR)/za-rtsig-handle-async.o $(LOOP_HANDLING_SIG_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) -lm

za-rtsig-wait-sync: $(OBJDIR)/za-rtsig-wait-sync.o $(LOOP_HANDLING_SIG_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) -lm

za-pthread-lifecycle: $(OBJDIR)/za-pthread-lifecycle.o $(OBJDIR)/util-timeval.o
	$(CC) -o $@ $^ $(CFLAGS) -lpthread -lm

za-pthread-cancel: $(OBJDIR)/za-pthread-cancel.o $(OBJDIR)/util-timeval.o
	$(CC) -o $@ $^ $(CFLAGS) -lpthread -lm

za-pthreads-condvar-sem: $(OBJDIR)/za-pthreads-condvar-sem.o $(OBJDIR)/util-ex-threads.o $(OBJDIR)/util-mutexattr.o
	$(CC) -o $@ $^ $(CFLAGS) -lpthread

za-pthreads-dup: $(OBJDIR)/za-pthreads-dup.o $(OBJDIR)/util-ex-threads.o
	$(CC) -o $@ $^ $(CFLAGS) -lpthread

za-pthreads-loop-errno-sig: $(OBJDIR)/za-pthreads-loop-errno-sig.o $(OBJDIR)/util-ex-threads.o $(LOOP_ERRNO_SIG_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) -lpthread

za-pthreads-sig: $(OBJDIR)/za-pthreads-sig.o $(OBJDIR)/util-ex-threads.o $(LOOP_HANDLING_SIG_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) -lpthread -lm


$(OBJDIR)/%.o: %.c $(DEPS) | $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS) $(INCLUDES)

## Create directory for object files if it does not exist:
##
.PHONY: $(OBJDIR)
$(OBJDIR):
	+@[ -d $@ ] || mkdir -p $@


.PHONY: clean
clean:
	-rm $(OBJDIR)/*.o
	-rm $(EXENAMES)


## EoF
