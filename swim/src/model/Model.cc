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
#include "Model.h"
#include <omnetpp.h>
#include <managers/execution/ExecutionManagerModBase.h>
#include "modules/PredictableRandomSource.h"
#include <sstream>
#include <math.h>
#include <iostream>
#include <assert.h>
#include <util/Utils.h>

using namespace std;

#define LOCDEBUG 0

Define_Module(Model);

const char* Model::HORIZON_PAR = "horizon";

Model::Model()
    : activeServerCountLast(0), timeActiveServerCountLast(0.0), activeServers(0)
{
}

Model::~Model() {
}

void Model::addExpectedChange(double time, ModelChange change)
{
    ModelChangeEvent event;
    event.startTime = simTime().dbl();
    event.time = time;
    event.change = change;
    events.insert(event);
}

void Model::removeExpectedChange()
{
    if (!events.empty()) {
        events.erase(--events.end());
    } else {
        std::cout << "removeExpectedChange(): serverCount "
                << configuration.getTotalActiveServers() << " activeServerCount "<< std::endl;
    }
}

int Model::getDimmerLevel() const {
    return 1 + (numberOfBrownoutLevels - 1) * configuration.getBrownOutLevel();
}

int Model::getActiveServerCountIn(double deltaTime, MTServerType::ServerType serverType)
{
    /*
     * We don't keep a past history, but if we need to know what was the active
     * server count an infinitesimal instant before now, we can use a negative delta time
     */
    ServerInfo* serverInfo = const_cast<ServerInfo*>(getServerInfoObj(serverType));
    if (deltaTime < 0) {
        return (timeActiveServerCountLast < simTime().dbl()) ? configuration.getActiveServers(serverType) : serverInfo->activeServerCountLast;
    }

    int servers = configuration.getActiveServers(serverType);

    double currentTime = simTime().dbl();
    ModelChangeEvents::iterator it = events.begin();
    while (it != events.end() && it->time <= currentTime + deltaTime) {
        if (it->change == getOnlineEventCode(serverType)) {
            servers++;
        }
        it++;
    }

    return servers;
}

void Model::setTrafficLoad(LoadBalancer::TrafficLoad serverA,
              LoadBalancer::TrafficLoad serverB, LoadBalancer::TrafficLoad serverC) {
    this->configuration.setTraffic(MTServerType::ServerType::A, serverA);
    this->configuration.setTraffic(MTServerType::ServerType::B, serverB);
    this->configuration.setTraffic(MTServerType::ServerType::C, serverC);
}

void Model::addServer(double bootDelay, MTServerType::ServerType serverType)
{
    ASSERT(!isServerBooting()); // only one add server tactic at a time
    addExpectedChange(simTime().dbl() + bootDelay, getOnlineEventCode(serverType));
    configuration.setBootRemain(ceil(bootDelay / evaluationPeriod), serverType);
    lastConfigurationUpdate = simTime();

#if LOCDEBUG
    std::cout << simTime().dbl() << ": " << "addServer(" << bootDelay << "): serverCount=" << getServers() << " active=" << activeServers << " expected=" << events.size() << std::endl;
#endif
}


void Model::serverBecameActive(MTServerType::ServerType serverType)
{
    ServerInfo* serverInfo = const_cast<ServerInfo* >(getServerInfoObj(serverType));
    serverInfo->activeServerCountLast = configuration.getActiveServers(serverType);
    timeActiveServerCountLast = simTime().dbl();

    /* remove expected change...assume it is the first SERVER_ONLINE */
    Model::ModelChange serverBootupEventCode = getOnlineEventCode(serverType);
    ModelChangeEvents::iterator it = events.begin();
    while (it != events.end() && it->change != serverBootupEventCode) {
        it++;
    }
    assert(it != events.end()); // there must be an expected change for this
    events.erase(it);

    configuration.setActiveServers(serverInfo->activeServerCountLast + 1, serverType);
    configuration.setBootRemain(0);
    lastConfigurationUpdate = simTime();

#if LOCDEBUG
    std::cout << simTime().dbl() << ": " << "serverBecameActive(): serverCount=" << getServers() << " active=" << activeServers << " expected=" << events.size() << std::endl;
    if (events.size() > 0) {
        cout << simTime().dbl() << "expected event time=" << events.begin()->time
                << string(((events.begin()->time > simTime().dbl()) ? " > " : " <= "))
                        << " current time" << endl;
    }
#endif

}

Model::ModelChange Model::getOnlineEventCode(MTServerType::ServerType serverType) const {
    ModelChange event = INVALID;

    switch (serverType) {
    case MTServerType::ServerType::A:
        event = SERVERA_ONLINE;
        break;
    case MTServerType::ServerType::B:
        event = SERVERB_ONLINE;
        break;
    case MTServerType::ServerType::C:
        event = SERVERC_ONLINE;
        break;
    case MTServerType::ServerType::NONE:
        assert(false);
    }

    return event;
}

void Model::removeServer(MTServerType::ServerType serverType)
{
    if (isServerBooting()) {

        /* the server we're removing is not active yet */
        removeExpectedChange();
        configuration.setBootRemain(0);
    } else {
        ServerInfo* serverInfo = const_cast<ServerInfo*>(getServerInfoObj(serverType));
        serverInfo->activeServerCountLast = configuration.getActiveServers(serverType);
        timeActiveServerCountLast = simTime().dbl();

        configuration.setActiveServers(serverInfo->activeServerCountLast - 1, serverType);
    }
    lastConfigurationUpdate = simTime();
    std::cout << "Server removed: " << serverType << std::endl;

#if LOCDEBUG
    std::cout << simTime().dbl() << ": " << "removeServer(): serverCount=" << getServers() << " active=" << activeServers << " expected=" << events.size() << std::endl;
#endif

}

void Model::setBrownoutFactor(int factor) {
    configuration.setBrownOutLevel(factor);
}

int Model::getBrownoutFactor() const {
    return configuration.getBrownOutLevel();
}

void Model::setDimmerFactor(double factor) {
    setBrownoutFactor(1.0 - factor);
}

double Model::getDimmerFactor() const {
    return 1.0 - getBrownoutFactor();
}

int const Model::getActiveServers() const {
    return activeServers;
}

int const Model::getServers() const {
    return (isServerBooting() ? activeServers + 1 : activeServers);
}

void Model::initialize(int stage) {
    if (stage == 0) {
        // get parameters
        serverA.maxServers = omnetpp::getSimulation()->getSystemModule()->par("maxServersA");
        serverB.maxServers = omnetpp::getSimulation()->getSystemModule()->par("maxServersB");
        serverC.maxServers = omnetpp::getSimulation()->getSystemModule()->par("maxServersC");
        int maxServers = serverA.maxServers + serverB.maxServers + serverC.maxServers;
        evaluationPeriod = getSimulation()->getSystemModule()->par("evaluationPeriod").doubleValue();
        bootDelay = Utils::getMeanAndVarianceFromParameter(
                                getSimulation()->getSystemModule()->par("bootDelay"));

        horizon = max(5.0,ceil(bootDelay / evaluationPeriod) * (maxServers - 1) + 1);
        if (hasPar(HORIZON_PAR)) {
            horizon = par("horizon");
        }
        if (horizon < 0) {
            horizon = max(5.0, ceil(bootDelay / evaluationPeriod) * (maxServers - 1) + 1);
        }

        numberOfBrownoutLevels = getSimulation()->getSystemModule()->par("numberOfBrownoutLevels");
        dimmerMargin = getSimulation()->getSystemModule()->par("dimmerMargin");
        lowerDimmerMargin = par("lowerDimmerMargin");
    } else {
        // start servers
        ExecutionManagerModBase* pExecMgr = check_and_cast<ExecutionManagerModBase*> (getParentModule()->getSubmodule("executionManager"));
        int initialServers = omnetpp::getSimulation()->getSystemModule()->par("initialServersA");
        while (initialServers > 0) {
            pExecMgr->addServerLatencyOptional(MTServerType::ServerType::A, true);
            initialServers--;
        }
        initialServers = omnetpp::getSimulation()->getSystemModule()->par( "initialServersB");
        while (initialServers > 0) {
            pExecMgr->addServerLatencyOptional(MTServerType::ServerType::B, true);
            initialServers--;
        }
        initialServers = omnetpp::getSimulation()->getSystemModule()->par("initialServersC");
        while (initialServers > 0) {
            pExecMgr->addServerLatencyOptional(MTServerType::ServerType::C, true);
            initialServers--;
        }
    }
}

bool Model::isServerBooting(MTServerType::ServerType serverType) const {
    bool isBooting = false;

    if (!events.empty()) {
        ModelChangeEvents::const_iterator eventIt = events.begin();
        if (eventIt != events.end()) {
            ModelChange changeEventCode = getOnlineEventCode(serverType);
            ASSERT(eventIt->change == changeEventCode);
            isBooting = true;
            eventIt++;
            ASSERT(eventIt == events.end()); // only one tactic should be active
        }
    }

    return isBooting;
}

bool Model::isServerBooting() const {
    bool isBooting = false;

    if (!events.empty()) {

        /* find if a server is booting. Assume only one can be booting */
        ModelChangeEvents::const_iterator eventIt = events.begin();
        if (eventIt != events.end()) {
            ModelChange changeEventCodeServerA = getOnlineEventCode(MTServerType::ServerType::A);
            ModelChange changeEventCodeServerB = getOnlineEventCode(MTServerType::ServerType::B);
            ModelChange changeEventCodeServerC = getOnlineEventCode(MTServerType::ServerType::C);
            ASSERT(eventIt->change == changeEventCodeServerA || eventIt->change == changeEventCodeServerB || eventIt->change == changeEventCodeServerC);
            isBooting = true;
            eventIt++;
            ASSERT(eventIt == events.end()); // only one tactic should be active
        }
    }
    return isBooting;
}


Configuration Model::getConfiguration() {
    Configuration configuration;

    if (events.empty()) {
        configuration.setBootRemain(0);
    } else {

        /* find if a server is booting. Assume only one can be booting */
        ModelChangeEvents::const_iterator eventIt = events.begin();
        if (eventIt != events.end()) {
            ASSERT(eventIt->change == SERVER_ONLINE);
            int bootRemain = ceil((eventIt->time - simTime().dbl()) / evaluationPeriod);

            /*
             * we never set boot remain to 0 here because the server could
             * still be booting (if we allowed random boot times)
             * so, we keep it > 0, and only serverBecameActive() can set it to 0
             */
            configuration.setBootRemain(std::max(1, bootRemain));
            eventIt++;
            ASSERT(eventIt == events.end()); // only one tactic should be active
        }
    }
    return configuration;
}

const Environment& Model::getEnvironment() const {
    return environment;
}

void Model::setEnvironment(const Environment& environment) {
    this->environment = environment;
}

int Model::getMaxServers(MTServerType::ServerType serverType) const {
    return getServerInfoObj(serverType)->maxServers;
}

const Model::ServerInfo* Model::getServerInfoObj(MTServerType::ServerType serverType) const {
    const ServerInfo* serverInfo = NULL;

    switch (serverType) {
    case MTServerType::ServerType::A:
        serverInfo = &serverA;
        break;
    case MTServerType::ServerType::B:
        serverInfo = &serverB;
        break;
    case MTServerType::ServerType::C:
        serverInfo = &serverC;
        break;
    case MTServerType::ServerType::NONE:
        assert(false);
    }

    return serverInfo;
}

double Model::getAvgResponseTime() const {
    return observations.avgResponseTime;
}

double Model::getEvaluationPeriod() const {
    return evaluationPeriod;
}

const Observations& Model::getObservations() const {
    return observations;
}

void Model::setObservations(const Observations& observations) {
    this->observations = observations;
}


double Model::getBootDelay() const {
    return bootDelay;
}

int Model::getHorizon() const {
    return horizon;
}

int Model::getNumberOfBrownoutLevels() const {
    return numberOfBrownoutLevels;
}

int Model::getNumberOfDimmerLevels() const {
    return numberOfBrownoutLevels;
}


double Model::getLowFidelityServiceTime(MTServerType::ServerType serverType) const {
    ServerInfo serverInfo;
    return getServerInfoObj(serverType)->lowFidelityServiceTime;
}

void Model::setLowFidelityServiceTime(double lowFidelityServiceTimeMean, double lowFidelityServiceTimeVariance, MTServerType::ServerType serverType) {
    ServerInfo* serverInfo = const_cast<ServerInfo*>(getServerInfoObj(serverType));
    serverInfo->lowFidelityServiceTime = lowFidelityServiceTimeMean;
    serverInfo->lowFidelityServiceTimeVariance = lowFidelityServiceTimeVariance;
}

int Model::getServerThreads() const {
    return serverThreads;
}

void Model::setServerThreads(int serverThreads) {
    this->serverThreads = serverThreads;
}

double Model::getServiceTime(MTServerType::ServerType serverType) const {
    return getServerInfoObj(serverType)->serviceTime;
}

void Model::setServiceTime(double serviceTimeMean, double serviceTimeVariance, MTServerType::ServerType serverType) {
    ServerInfo* serverInfo = const_cast<ServerInfo*>(getServerInfoObj(serverType));
    serverInfo->serviceTime = serviceTimeMean;
    serverInfo->serviceTimeVariance = serviceTimeVariance;
}

double Model::getLowFidelityServiceTimeVariance(MTServerType::ServerType serverType) const {
    return getServerInfoObj(serverType)->lowFidelityServiceTimeVariance;
}

double Model::getServiceTimeVariance(MTServerType::ServerType serverType) const {
    return getServerInfoObj(serverType)->serviceTimeVariance;
}

double Model::brownoutLevelToFactor(int brownoutLevel) const {
    if (lowerDimmerMargin) {

        // lower dimmer margin is upper brownout margin
        return (1.0 - dimmerMargin) * (brownoutLevel - 1.0) / (getNumberOfBrownoutLevels() - 1.0);
    }
    return dimmerMargin + (1.0 - 2 * dimmerMargin) * (brownoutLevel - 1.0) / (getNumberOfBrownoutLevels() - 1.0);
}

int Model::brownoutFactorToLevel(double brownoutFactor) const {
    if (lowerDimmerMargin) {

        // lower dimmer margin is upper brownout margin
        return 1 + round(brownoutFactor * (getNumberOfBrownoutLevels() - 1) / (1.0 - dimmerMargin));
    }
    return 1 + round((brownoutFactor - dimmerMargin) * (getNumberOfBrownoutLevels() - 1) / (1.0 - 2 * dimmerMargin));
}

void Model::setJobServerInfo(string jobName, MTServerType::ServerType serverType) {
    JobServeInfo::iterator itr = jobServeInfo.find(jobName);

    assert(itr == jobServeInfo.end());
    jobServeInfo[jobName] = serverType;
    //cout << "JobName = " << jobName << endl;
}

    MTServerType::ServerType Model::getJobServerInfo(string jobName) {
    JobServeInfo::iterator itr = jobServeInfo.find(jobName);
    MTServerType::ServerType serverType = MTServerType::ServerType::NONE;

    if (itr != jobServeInfo.end()) {
        serverType = itr->second;
    }

    return serverType;
}

bool Model::isDimmerMarginLower() const {
    return lowerDimmerMargin;
}

double Model::dimmerLevelToFactor(int dimmerLevel) const {
    int brownoutLevel = getNumberOfBrownoutLevels() - dimmerLevel + 1;

    return brownoutLevelToFactor(brownoutLevel);
}

int Model::dimmerFactorToLevel(double dimmerFactor) const {
    return brownoutFactorToLevel(1.0 - dimmerFactor);
}


double Model::getDimmerMargin() const {
    return dimmerMargin;
}
