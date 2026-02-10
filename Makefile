run: compile
	./main 0.0.0.0 8081

compile: main.c clean
	gcc main.c -o main

clean:
	rm main
