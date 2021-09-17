---
title: COL334 Assignment 2 Documentation
author: Arpit Saxena, 2018MT10742
geometry: margin=2cm
output: pdf_document
toc: true
---

## Introduction

The aim of this report is to document implementation defined behaviors in cases where the assignment spec was ambiguous or didn't mention anything.

## Registration

The spec says that:

> The user registration should be the very first activity that is done when a
client opens the sockets to the server. The server should return an error message
to all other commands it receives on the socket until the user registration has
been done on both the sockets.

It is not specified what to do if a receive socket is opened and some other user sends a message to it. In this implementation, we forward the message to the client if it has registered a TORECV socket.

## Message Sending from Client to Server

If recipient of a message sends an error message back to the server, the server sends back "ERROR 102 Unable to send" to the sender. This makes sense because sending a Header incomplete would indicate to the sender there was an issue at it's end. But Unable to send could mean the recipient was not found or that there was some error in forwarding the message to it. It would've been clearer had there been more errors specified, but this makes the most sense in the scenario.

## Broadcast messages

### Meaning of Broadcast

Since it is unclear in the specification whether a broadcast message sent by a user would be sent back to it or not, a conscious choice has been made to send the message to all registered users except the sending user. This option was deemed most logical and hence used.

### Broadcasting Method

It has been clarified that the server could use either a stop-and-wait method or spawn multiple threads for forwarding broadcast messages. In this implementation, the stop-and-wait method has been chosen owing to it's simplicity.

## Client Application

### Inputting username and server IP

Since it's not very clearly specified how exactly the client application should input the username and server address, here is what this implementation is doing:

We first print out a prompt to enter username, then read the entire line i.e. read everything up to `\n` and take that as the username. After that the user is prompted to enter the server address which is input in the same way.

### Threads

Instead of _starting_ two threads as the spec says, we start one thread and reuse the main thread for the other functionality. This way we have lesser number of threads spawned and all of them are doing some work, even though most of it is waiting for IO.

## Server Application

### Receiving bad ack messages

Upon receiving an error or a bad (read not according to spec) acknowledgment message from the receiver, the server sends ERROR 102 to the sender. If the error message is ERROR 103 or there is a bad ack, the server also closes the receiver socket. This makes sense because it means that the connection to the recipient has somehow gone bad and we don't have any way to recover from it.

### Empty Username

The specification is not clear on if an empty string is valid as a username or not. As a design choice, this implementation labels empty usernames as malformed. Allowing empty usernames doesn't make any sense and so they are disallowed.
