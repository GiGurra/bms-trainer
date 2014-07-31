#include "DecodeServer.h"

#include "libgurra/Thread.h"

int main() {

	DecodeServer server;
	server.start();

	// Thread::sleep(1000);

	// server.kill();
	server.join();

	return 0;
}
