all:
	$(CC) main.c lalias.c -o lalias -fsanitize=undefined

run:
	./lalias

