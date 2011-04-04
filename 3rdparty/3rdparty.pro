libs3.depends =
libs3.commands = cd libs3; \
        make -f GNUmakefile.osx; \
        cp build/lib/libs3.a ..; \
        cd ..
libs3.target = libs3.a

all.depends = libs3.a
all.commands =
all.CONFIG = phony

TARGET = \\\

CONFIG -= app_bundle

QMAKE_EXTRA_TARGETS += libs3 all
