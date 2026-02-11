CC = cc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lm

# Directories
SRCDIR = src
LIBDIR = lib/swisseph
TESTDIR = tests
BUILDDIR = build

# Swiss Ephemeris sources
SWE_SRCS = $(LIBDIR)/swecl.c $(LIBDIR)/swedate.c $(LIBDIR)/sweephe4.c \
           $(LIBDIR)/swehel.c $(LIBDIR)/swehouse.c $(LIBDIR)/swejpl.c \
           $(LIBDIR)/swemmoon.c $(LIBDIR)/swemplan.c $(LIBDIR)/sweph.c \
           $(LIBDIR)/swephlib.c
SWE_OBJS = $(patsubst $(LIBDIR)/%.c,$(BUILDDIR)/swe/%.o,$(SWE_SRCS))

# Our sources (excluding main.c for test builds)
APP_SRCS = $(SRCDIR)/astro.c $(SRCDIR)/date_utils.c $(SRCDIR)/tithi.c \
           $(SRCDIR)/masa.c $(SRCDIR)/panchang.c $(SRCDIR)/solar.c
APP_OBJS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(APP_SRCS))
MAIN_OBJ = $(BUILDDIR)/main.o

# Test sources
TEST_SRCS = $(wildcard $(TESTDIR)/test_*.c)
TEST_BINS = $(patsubst $(TESTDIR)/%.c,$(BUILDDIR)/%,$(TEST_SRCS))

# Include paths
INCLUDES = -I$(LIBDIR) -I$(SRCDIR)

# Target binary
TARGET = hindu-calendar

.PHONY: all clean test

all: $(BUILDDIR) $(TARGET)

$(TARGET): $(SWE_OBJS) $(APP_OBJS) $(MAIN_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Swiss Ephemeris objects
$(BUILDDIR)/swe/%.o: $(LIBDIR)/%.c | $(BUILDDIR)/swe
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Application objects
$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Test binaries
$(BUILDDIR)/test_%: $(TESTDIR)/test_%.c $(SWE_OBJS) $(APP_OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(SWE_OBJS) $(APP_OBJS) $(LDFLAGS)

# Directories
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/swe:
	mkdir -p $(BUILDDIR)/swe

test: $(TEST_BINS)
	@for t in $(TEST_BINS); do \
		echo "=== Running $$t ==="; \
		./$$t || exit 1; \
		echo ""; \
	done

clean:
	rm -rf $(BUILDDIR) $(TARGET)
