#pragma once

#include "network/NetworkModule.h"
#include "network/messages/ClientMessages.h"

#include "UserConnectHandler.h"
#include "UserDisconnectHandler.h"
#include "AttackHandler.h"
#include "MoveHandler.h"

namespace backend {

class ServerNetworkModule: public NetworkModule {
	void configureHandlers() const override {
		configureHandler(Type_UserConnect, UserConnectHandler(network::Network &, backend::EntityStorage &));
		configureHandler(Type_UserDisconnect, UserDisconnectHandler());
		configureHandler(Type_Attack, AttackHandler());
		configureHandler(Type_Move, MoveHandler());
	}
};

}
