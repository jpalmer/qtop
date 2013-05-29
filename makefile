a.out:qtop.c qtop.h print.c print.h
	gcc --std=gnu99 -g  -Wall -Wextra --pedantic -I /usr/physics/torque/include -L /usr/physics/torque/lib -ltorque print.c qtop.c
	
