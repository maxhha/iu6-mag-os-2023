CFLAGS := --std=gnu17 -Wall -Werror -Wfloat-equal -Wfloat-conversion -Wpedantic
CFLAGS += $(cflags)
LFLAGS := -lpthread

SRCS := $(filter-out $(wildcard src/*.tmpl.c), $(wildcard src/*.c))

OBJS := $(SRCS:src/%.c=out/%.o)

CC := gcc

emmet.exe : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LFLAGS)

out :
	mkdir out

out/%.o : ./src/%.c | ./out
	$(CC) $(CFLAGS) $(flags) -o $@ -c $<

.PHONY : clean
clean : | ./out
	@rm -f *.exe out/*.o

# .PHONY : clean
# dev :
# 	@clear
# 	@make -s run
# 	@inotifywait -e close_write -mq src/ inc/ | \
# 		while read -r dir evts file; do clear; make -s run; done
