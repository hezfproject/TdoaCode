LIBTARGET= libgeneral_common_2g.a

SRCS= numtrans.c

CROSS_COMPILE := arm-linux-
CC=$(CROSS_COMPILE)gcc
CPP=$(CROSS_COMPILE)g++
LD=$(CROSS_COMPILE)ld

#CPPFLAGS += mineapp/
CPPFLAGS += -Wall -Werror -g
CPPFLAGS += -DDEBUG -D__ARM32__

OBJS+= $(SRCS:.c=.o)
DEPS+= $(SRCS:.c=.d)

$(LIBTARGET): $(OBJS)
	ar rcs $@ $(OBJS)

%.o: %.c
	@echo "====> Compiling $@"
	$(CPP) $(CPPFLAGS) -c -o $@ $<

%.o: %.cc
	@echo "====> Compiling $@"
	$(CPP) $(CPPFLAGS) -c -o $@ $<

%.o: %.cpp
	@echo "====> Compiling $@"
	$(CPP) $(CPPFLAGS) -c -o $@ $<

%.d: %.c
	@echo "====> Compiling $@"
	@$(CPP) $(CPPFLAGS) -MM $< |\
	sed -e "s/$*\.o/$*.o $*.d/" > $@

%.d: %.cc
	@echo "====> Compiling $@"
	@$(CPP) $(CPPFLAGS) -MM $< |\
	sed -e "s/$*\.o/$*.o $*.d/" > $@

%.d: %.cpp
	@echo "====> Compiling $@"
	@$(CPP) $(CPPFLAGS) -MM $< |\
	sed -e "s/$*\.o/$*.o $*.d/" > $@

include $(DEPS)

clean:
	rm -rf *.o *.d *.a *.exe $(LIBTARGET)
