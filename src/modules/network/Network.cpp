#include "Network.h"
#include "IProtocolHandler.h"
#include "ProtocolHandlerRegistry.h"
#include "NetworkEvents.h"
#include "network/messages/ClientMessages.h"
#include "network/messages/ServerMessages.h"
#include "core/Trace.h"
#include "core/Log.h"

#include <memory>

namespace network {

Network::Network(ProtocolHandlerRegistryPtr protocolHandlerRegistry, core::EventBusPtr eventBus) :
		_protocolHandlerRegistry(protocolHandlerRegistry), _eventBus(eventBus), _server(nullptr), _client(nullptr) {
}

Network::~Network() {
	enet_host_destroy(_server);
	enet_host_destroy(_client);
	enet_deinitialize();
}

bool Network::start() {
	if (enet_initialize() != 0)
		return false;
	enet_time_set(0);
	return true;
}

bool Network::bind(uint16_t port, const std::string& hostname, int maxPeers, int maxChannels) {
	if (_server)
		return false;
	if (maxPeers <= 0)
		return false;
	if (maxChannels <= 0)
		return false;
	ENetAddress address;
	if (hostname.empty())
		address.host = ENET_HOST_ANY;
	else
		enet_address_set_host(&address, hostname.c_str());
	address.port = port;
	_server = enet_host_create(&address, maxPeers, maxChannels, 0, 0);
	if (_server == nullptr) {
		return false;
	}
	return true;
}

ENetPeer* Network::connect(uint16_t port, const std::string& hostname, int maxChannels) {
	if (_client)
		disconnect();
	_client = enet_host_create(nullptr, 1, maxChannels, 57600 / 8, 14400 / 8);
	if (_client == nullptr) {
		return nullptr;
	}

	ENetAddress address;
	enet_address_set_host(&address, hostname.c_str());
	address.port = port;

	ENetPeer *peer = enet_host_connect(_client, &address, maxChannels, 0);
	enet_host_flush(_client);
	enet_peer_timeout(peer, ENET_PEER_TIMEOUT_LIMIT, ENET_PEER_TIMEOUT_MINIMUM, ENET_PEER_TIMEOUT_MAXIMUM);
	return peer;
}

void Network::disconnect() {
	if (_client == nullptr)
		return;

	for (size_t i = 0; i < _client->peerCount; ++i) {
		ENetPeer *peer = &_client->peers[i];
		disconnectPeer(peer);
	}
	enet_host_destroy(_client);
	_client = nullptr;
}

void Network::disconnectPeer(ENetPeer *peer, uint32_t timeout) {
	if (peer == nullptr)
		return;
	Log::info("trying to disconnect peer %i", peer->connectID);
	ENetEvent event;
	enet_peer_disconnect(peer, 0);
	bool success = false;
	/* Wait some time for the disconnect to succeed
	 * and drop any packets received packets. */
	while (enet_host_service(_client, &event, timeout) > 0) {
		switch (event.type) {
		case ENET_EVENT_TYPE_RECEIVE:
			enet_packet_destroy(event.packet);
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			success = true;
			break;
		case ENET_EVENT_TYPE_CONNECT:
		case ENET_EVENT_TYPE_NONE:
			break;
		}
	}
	if (!success) {
		/* We've arrived here, so the disconnect attempt didn't */
		/* succeed yet. Force the connection down. */
		enet_peer_reset(peer);
	}
}

bool Network::packetReceived(ENetEvent& event, bool server) {
	flatbuffers::Verifier v(event.packet->data, event.packet->dataLength);

	if (!server) {
		if (!messages::server::VerifyServerMessageBuffer(v)) {
			Log::error("Illegal server packet received with length: %i", (int)event.packet->dataLength);
			return false;
		}
		const messages::server::ServerMessage *req = messages::server::GetServerMessage(event.packet->data);
		messages::server::Type type = req->data_type();
		ProtocolHandlerPtr handler = _protocolHandlerRegistry->getHandler(type);
		if (!handler) {
			Log::error("No handler for server msg type %s", messages::server::EnumNameType(type));
			return false;
		}
		Log::debug("Received %s", messages::server::EnumNameType(type));
		handler->execute(event.peer, req->data());
		return true;
	}

	if (!messages::client::VerifyClientMessageBuffer(v)) {
		Log::error("Illegal client packet received with length: %i", (int)event.packet->dataLength);
		return false;
	}
	const messages::client::ClientMessage *req = messages::client::GetClientMessage(event.packet->data);
	messages::client::Type type = req->data_type();
	ProtocolHandlerPtr handler = _protocolHandlerRegistry->getHandler(type);
	if (!handler) {
		Log::error("No handler for client msg type %s", messages::client::EnumNameType(type));
		return false;
	}
	Log::debug("Received %s", messages::client::EnumNameType(type));
	handler->execute(event.peer, req->data());
	return true;
}

void Network::updateHost(ENetHost* host, bool server) {
	if (host == nullptr)
		return;
	ENetEvent event;
	while (enet_host_service(host, &event, 0) > 0) {
		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT: {
			_eventBus->publish(NewConnectionEvent(event.peer));
			break;
		}
		case ENET_EVENT_TYPE_RECEIVE: {
			if (!packetReceived(event, server)) {
				Log::error("Failure while receiving a package - disconnecting now...");
				disconnectPeer(event.peer, 10);
			}
			enet_packet_destroy(event.packet);
			break;
		}
		case ENET_EVENT_TYPE_DISCONNECT: {
			_eventBus->publish(DisconnectEvent(event.peer));
			break;
		}
		case ENET_EVENT_TYPE_NONE: {
			break;
		}
		}
	}
}

void Network::update() {
	core_trace_scoped("Network");
	updateHost(_server, true);
	updateHost(_client, false);
}

}
