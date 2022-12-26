/*******************************************************************************
 * Simulator of Web Infrastructure and Management
 * Copyright (c) 2016 Carnegie Mellon University.
 * All Rights Reserved.
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS," WITH NO WARRANTIES WHATSOEVER. CARNEGIE
 * MELLON UNIVERSITY EXPRESSLY DISCLAIMS TO THE FULLEST EXTENT PERMITTED BY LAW
 * ALL EXPRESS, IMPLIED, AND STATUTORY WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, AND NON-INFRINGEMENT OF PROPRIETARY RIGHTS.
 *  
 * Released under a BSD license, please see license.txt for full terms.
 * DM-0003883
 *******************************************************************************/
#include "Configuration.h"
#include <typeinfo>
#include <modules/LoadBalancer.h>
#include <modules/MTServerType.h>
//    int brownoutLevel = 1 + (pModel->getNumberOfBrownoutLevels() - 1) * pModel->getConfiguration().getBrownOutFactor();


Configuration::Configuration() : 
        serversA(0), serversB(0), serversC(0),
        bootRemain(0), 
        bootServerType(MTServerType::NONE), 
        brownoutLevel(0),
        trafficA(LoadBalancer::TrafficLoad::HUNDRED),
        trafficB(LoadBalancer::TrafficLoad::ZERO), 
        trafficC(LoadBalancer::TrafficLoad::ZERO) {}

Configuration::Configuration(int serverA, int serverB, int serverC,
        int bootRemain, MTServerType::ServerType serverType,
        int brownoutLevel, LoadBalancer::TrafficLoad trafficA, 
        LoadBalancer::TrafficLoad trafficB, LoadBalancer::TrafficLoad trafficC) :
        serversA(serverA), serversB(serverB), serversC(serverC),
                bootRemain(bootRemain), bootServerType(serverType),
                brownoutLevel(brownoutLevel), trafficA(trafficA),
                trafficB(trafficB), trafficC(trafficC) {};


int Configuration::getBootRemain() const {
    return bootRemain;
}

int Configuration::getServers(MTServerType::ServerType type) const {
    int cntServers;

    switch (type) {
    case MTServerType::ServerType::POWERFUL:
        cntServers = this->serversA + ((bootServerType == MTServerType::ServerType::POWERFUL) ? 1 : 0);
        break;
    case MTServerType::ServerType::AVERAGE:
        cntServers = this->serversB + ((bootServerType == MTServerType::ServerType::AVERAGE) ? 1 : 0);
        break;
    case MTServerType::ServerType::WEAK:
        cntServers = this->serversC + ((bootServerType == MTServerType::ServerType::WEAK) ? 1 : 0);
        break;
    case MTServerType::ServerType::NONE:
        cntServers = 0;
    }

    return cntServers;
}

MTServerType::ServerType Configuration::getBootType() const {
    return bootServerType;
}

void Configuration::setBootRemain(int bootRemain, MTServerType::ServerType serverType) {
    this->bootRemain = bootRemain;

    if (this->bootRemain == 0) {
        this->bootServerType = MTServerType::ServerType::NONE;
    } else if (serverType != MTServerType::ServerType::NONE) {
        this->bootServerType = serverType;
    }
}

int Configuration::getBrownOutLevel() const {
    return brownoutLevel;
}

void Configuration::setBrownOutLevel(int brownoutLevel) {
    this->brownoutLevel = brownoutLevel;
}

int Configuration::getActiveServers(MTServerType::ServerType serverType) const {
    int cntServer;

    switch (serverType) {
    case MTServerType::ServerType::POWERFUL:
        cntServer = this->serversA;
        break;
    case MTServerType::ServerType::AVERAGE:
        cntServer = this->serversB;
        break;
    case MTServerType::ServerType::WEAK:
        cntServer = this->serversC;
        break;
    case MTServerType::ServerType::NONE:
        cntServer = 0;
    }

    return cntServer;
}

void Configuration::setActiveServers(int servers, MTServerType::ServerType serverType) {
    switch (serverType) {
    case MTServerType::ServerType::POWERFUL:
        this->serversA = servers;
        break;
    case MTServerType::ServerType::AVERAGE:
        this->serversB = servers;
        break;
    case MTServerType::ServerType::WEAK:
        this->serversC = servers;
        break;
    case MTServerType::ServerType::NONE:
        assert(false);
    }

    this->bootRemain = 0;
    this->bootServerType = MTServerType::ServerType::NONE;
}

LoadBalancer::TrafficLoad Configuration::getTraffic(MTServerType::ServerType serverType) const {
    LoadBalancer::TrafficLoad traffic = LoadBalancer::TrafficLoad::INVALID;

    switch (serverType) {
    case MTServerType::ServerType::POWERFUL:
        traffic = this->trafficA;
        break;
    case MTServerType::ServerType::AVERAGE:
        traffic = this->trafficB;
        break;
    case MTServerType::ServerType::WEAK:
        traffic = this->trafficC;
        break;
    case MTServerType::ServerType::NONE:
        assert(false);
    }

    return traffic;
}

void Configuration::setTraffic(MTServerType::ServerType serverType, LoadBalancer::TrafficLoad trafficLoad) {
    switch (serverType) {
        case MTServerType::ServerType::POWERFUL:
            this->trafficA = trafficLoad;
            break;
        case MTServerType::ServerType::AVERAGE:
            this->trafficB = trafficLoad;
            break;
        case MTServerType::ServerType::WEAK:
            this->trafficC = trafficLoad;
            break;
        case MTServerType::ServerType::NONE:
            assert(false);
        }
}

int Configuration::getTotalActiveServers() const {
    return this->serversA + this->serversB + this->serversC;
}
