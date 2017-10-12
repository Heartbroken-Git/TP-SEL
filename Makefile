all: interceptable intercepteur docs

interceptable:
	gcc src/interceptable.c -o bin/interceptable -Wall -Werror
intercepteur:
	g++ -std=c++11 src/intercepteur.cpp -o bin/intercepteur -Wall -Werror
docs:
	doxygen Doxyfile
clean:
	rm -r -f *.o
mrproper:
	clean
	rm -f bin/intercepteur
	rm -f bin/interceptable
	rm -f -r docs/*

