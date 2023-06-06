#!/bin/sh
g++ client.cpp -o client
g++ server.cpp -lpthread -o server