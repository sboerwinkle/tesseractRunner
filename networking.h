extern char networkActive;

extern void initNetwork(int argc, char **argv);

extern void netTick();

extern void stopNetwork();

#define USAGE_MSG "Usage: <port>  < h <numClients> | c <ip> >"
