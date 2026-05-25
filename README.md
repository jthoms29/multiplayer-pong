John Thoms, gvr812, 11357558

# Usage
Just run `make`

## Servers:
```
./<protocol>_server <Port>
```
The game loop will not start until two clients connect. The first to connect will control the left paddle, and the
second will control the right paddle. Control with the mouse wheel.

## Clients:
```
./<protocol>_client <Server Hostname> <Server Port>
```

## Hosts:
In this implementation the game logic is handled on the side of one of the players

```
./<protocol>_host <Port>
The game loop will not start until one client connects


# Test file
```
./test
```
Basic unit tests for the game logic

# Data
The data folder contains latency info gathered from playing the different implementations, as well as an R script which graphs the latency data.
