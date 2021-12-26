#! /bin/sh

rm server
#rm client

g++ -o server fileserver.cpp -lpthread
#g++ -o client LinClient.cpp


