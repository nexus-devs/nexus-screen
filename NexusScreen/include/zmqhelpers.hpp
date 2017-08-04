#pragma once

#include "zmq.hpp"

//  Convert string to 0MQ string and send to socket
static bool
s_send(zmq::socket_t & socket, const std::string & string) {

	zmq::message_t message(string.size());
	memcpy(message.data(), string.data(), string.size());

	bool rc = socket.send(message);
	return (rc);
}

//  Sends string as 0MQ string, as multipart non-terminal
static bool
s_sendmore(zmq::socket_t & socket, const std::string & string) {

	zmq::message_t message(string.size());
	memcpy(message.data(), string.data(), string.size());

	bool rc = socket.send(message, ZMQ_SNDMORE);
	return (rc);
}
