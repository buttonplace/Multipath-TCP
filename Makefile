all: projectServer projectClient

projectServer: projectServer.c
	gcc -o projectServer projectServer.c

projectClient: projectClient.c
	gcc -o projectClient projectClient.c
