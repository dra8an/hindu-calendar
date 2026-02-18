CC = cc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lm

# Directories
SRCDIR = src
TESTDIR = tests
BUILDDIR = build

# Ephemeris backend selection: default = moshier, USE_SWISSEPH=1 = Swiss Ephemeris
ifdef USE_SWISSEPH
  CFLAGS += -DUSE_SWISSEPH
  LIBDIR = lib/swisseph
  SWE_SRCS = $(LIBDIR)/swecl.c $(LIBDIR)/swedate.c $(LIBDIR)/sweephe4.c \
             $(LIBDIR)/swehel.c $(LIBDIR)/swehouse.c $(LIBDIR)/swejpl.c \
             $(LIBDIR)/swemmoon.c $(LIBDIR)/swemplan.c $(LIBDIR)/sweph.c \
             $(LIBDIR)/swephlib.c
  EPH_OBJS = $(patsubst $(LIBDIR)/%.c,$(BUILDDIR)/swe/%.o,$(SWE_SRCS))
  INCLUDES = -I$(LIBDIR) -I$(SRCDIR)
  EPH_OBJDIR = $(BUILDDIR)/swe
else
  LIBDIR = lib/moshier
  MOSH_SRCS = $(wildcard $(LIBDIR)/*.c)
  EPH_OBJS = $(patsubst $(LIBDIR)/%.c,$(BUILDDIR)/moshier/%.o,$(MOSH_SRCS))
  INCLUDES = -I$(LIBDIR) -I$(SRCDIR)
  EPH_OBJDIR = $(BUILDDIR)/moshier
endif

# Our sources (excluding main.c for test builds)
APP_SRCS = $(SRCDIR)/astro.c $(SRCDIR)/date_utils.c $(SRCDIR)/tithi.c \
           $(SRCDIR)/masa.c $(SRCDIR)/panchang.c $(SRCDIR)/solar.c
APP_OBJS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(APP_SRCS))
MAIN_OBJ = $(BUILDDIR)/main.o

# Test sources
TEST_SRCS = $(wildcard $(TESTDIR)/test_*.c)
TEST_BINS = $(patsubst $(TESTDIR)/%.c,$(BUILDDIR)/%,$(TEST_SRCS))

# Generator sources
GEN_REF_SRC = tools/generate_ref_data.c
GEN_SOLAR_SRC = tools/gen_solar_ref.c

# Target binary
TARGET = hindu-calendar

.PHONY: all clean test gen-ref gen-json

all: $(BUILDDIR) $(TARGET)

$(TARGET): $(EPH_OBJS) $(APP_OBJS) $(MAIN_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Ephemeris library objects
$(BUILDDIR)/swe/%.o: lib/swisseph/%.c | $(BUILDDIR)/swe
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILDDIR)/moshier/%.o: lib/moshier/%.c | $(BUILDDIR)/moshier
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Application objects
$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Test binaries
$(BUILDDIR)/test_%: $(TESTDIR)/test_%.c $(EPH_OBJS) $(APP_OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(EPH_OBJS) $(APP_OBJS) $(LDFLAGS)

# Directories
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/swe:
	mkdir -p $(BUILDDIR)/swe

$(BUILDDIR)/moshier:
	mkdir -p $(BUILDDIR)/moshier

test: $(TEST_BINS)
	@for t in $(TEST_BINS); do \
		echo "=== Running $$t ==="; \
		./$$t || exit 1; \
		echo ""; \
	done

# Generator binaries
$(BUILDDIR)/gen_ref: $(GEN_REF_SRC) $(EPH_OBJS) $(APP_OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(EPH_OBJS) $(APP_OBJS) $(LDFLAGS)

$(BUILDDIR)/gen_solar_ref: $(GEN_SOLAR_SRC) $(EPH_OBJS) $(APP_OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(EPH_OBJS) $(APP_OBJS) $(LDFLAGS)

ifdef USE_SWISSEPH
  GEN_BACKEND = se
else
  GEN_BACKEND = moshier
endif

gen-ref: $(BUILDDIR)/gen_ref $(BUILDDIR)/gen_solar_ref
	mkdir -p validation/$(GEN_BACKEND)
	./$(BUILDDIR)/gen_ref -o validation/$(GEN_BACKEND)
	./$(BUILDDIR)/gen_solar_ref -o validation/$(GEN_BACKEND)
	python3 tools/extract_adhika_kshaya.py validation/$(GEN_BACKEND)/ref_1900_2050.csv validation/$(GEN_BACKEND)/adhika_kshaya_tithis.csv

gen-json:
	python3 tools/csv_to_json.py --backend $(GEN_BACKEND)
	python3 tools/csv_to_solar_json.py --backend $(GEN_BACKEND)

clean:
	rm -rf $(BUILDDIR) $(TARGET)
