#Makefile for Pokegame

ground: genGround.c
	gcc genGround.c -o genground

run: genground
	./genground

clean: 
	rm -f genground
