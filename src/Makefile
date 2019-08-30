# Tarek Elseify
# March 11, 2019
# Lab 3 - Networked Spell Checker
# CIS 3207 SEC 001
# tug35668@temple.edu
#
# This is the makefile for the spell checker

TARGET=server
CC=gcc
CFLAGS=-lpthread -pthread

all: clean server

server: main.c queue.c simple_server.h open_listenfd.c
	$(CC) $(CFLAGS) -o server main.c queue.c open_listenfd.c

clean:
	-rm -f *~
	-rm -f *.o
	-rm -f $(TARGET)
