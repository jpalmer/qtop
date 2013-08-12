a.out:qtop.c qtop.h print.c print.h
	gcc --std=gnu99 -g  -Wall -Wextra  -I /usr/physics/torque/include -L /usr/physics/torque/lib -ltorque -ltermcap -lm print.c qtop.c
	
