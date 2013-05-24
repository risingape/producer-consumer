all: clean producerconsumer

producerconsumer:
		$(CC) -lrt -lpthread consumer.c -o consumer
		$(CC) -lrt -lpthread producer.c -o producer

clean:
		$(RM) consumer producer *.o
