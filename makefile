a.out:qtop.c qtop.h print.c print.h
	gcc --std=gnu99 -g  -Wall -Wextra  -I /usr/physics/torque/include -L /usr/physics/torque/lib -ltorque  -lm print.c qtop.c
install:a.out
	cp a.out /usr/physics/qtop/qtop

localinstall: a.out
	cp a.out /usr/bin/qtop
