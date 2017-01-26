#!/bin/bash
gcc quizzClient.c -o quizzClient.bin -ggdb -O0
gcc quizzServer.c -o quizzServer.bin -pthread -lsqlite3 -ggdb -O0
sqlite3 questions.db < questions.sql
