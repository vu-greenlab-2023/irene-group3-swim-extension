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
#include "ExecutionManagerModBase.h"
#include <sstream>
#include <cstdlib>
#include <boost/tokenizer.hpp>

using namespace std;
using namespace omnetpp;

const char* ExecutionManagerModBase::SIG_SERVER_REMOVED = "serverRemoved";
const char* ExecutionManagerModBase::SIG_SERVER_ADDED = "serverAdded";
const char* ExecutionManagerModBase::SIG_SERVER_ACTIVATED = "serverActivated";
const char* ExecutionManagerModBase::SIG_BROWNOUT_SET = "brownoutSet";


ExecutionManagerModBase::ExecutionManagerModBase() : serverRemoveInProgress(0), testMsg(0) {
}

void ExecutionManagerModBase::initialize() {
    pModel = check_and_cast<Model*> (getParentModule()->getSubmodule("model"));
    serverRemovedSignal = registerSignal(SIG_SERVER_REMOVED);
    serverAddedSignal = registerSignal(SIG_SERVER_ADDED);
    serverActivatedSignal = registerSignal(SIG_SERVER_ACTIVATED);
    brownoutSetSignal = registerSignal(SIG_BROWNOUT_SET);
//    testMsg = new cMessage;
//    testMsg->setKind(0);
//    scheduleAt(simTime() + 1, testMsg);
}

void ExecutionManagerModBase::handleMessage(cMessage* msg) {
    if (msg == testMsg) {
        if (msg->getKind() == 0) {
            addServer();
            msg->setKind(1);
            scheduleAt(simTime() + 31, msg);
        } else {
            removeServer();
        }
        return;
    }
    BootComplete* bootComplete = check_and_cast<BootComplete *>(msg);

    doAddServerBootComplete(bootComplete);

    //  notify add complete to model
    cModule *server = omnetpp::getSimulation()->getModule(bootComplete->getModuleId());
    MTServerType::ServerType serverType = getServerTypeFromName(server->getFullName());
    pModel->serverBecameActive(serverType);
    emit(serverActivatedSignal, true);

    cout << "t=" << simTime() << " addServer() complete" << endl;

    // remove from pending
    BootCompletes::iterator it = pendingMessages.find(bootComplete);
    ASSERT(it != pendingMessages.end());
    delete *it;
    pendingMessages.erase(it);
}

ExecutionManagerModBase::~ExecutionManagerModBase() {
    for (BootCompletes::iterator it = pendingMessages.begin(); it != pendingMessages.end(); ++it) {
        cancelAndDelete(*it);
    }
    cancelAndDelete(testMsg);
}

void ExecutionManagerModBase::addServer(MTServerType::ServerType serverType) {
    cout << "t=" << simTime() << " executing addServer()" << endl;
    addServerLatencyOptional(serverType);
}

void ExecutionManagerModBase::addServerLatencyOptional(MTServerType::ServerType serverType, bool instantaneous) {
    Enter_Method("addServer()");
    int serverCount = pModel->getConfiguration().getServers(serverType);
    ASSERT(serverCount < pModel->getConfiguration().getMaxServers(serverType));

    BootComplete* bootComplete = doAddServer(serverType, instantaneous);

    double bootDelay = 0;
    if (!instantaneous) {
        bootDelay = omnetpp::getSimulation()->getSystemModule()->par("bootDelay");
        cout << "adding server with latency=" << bootDelay << endl;
    }

    pModel->addServer(bootDelay, serverType);
    //TODO: try to put true
    emit(serverAddedSignal, serverType);

    pendingMessages.insert(bootComplete);
    if (bootDelay == 0) {
        handleMessage(bootComplete);
    } else {
        scheduleAt(simTime() + bootDelay, bootComplete);
    }
}

void ExecutionManagerModBase::removeServer(MTServerType::ServerType serverType) {
    Enter_Method("removeServer()");
    cout << "t=" << simTime() << " executing removeServer()" << endl;
    int serverCount = pModel->getConfiguration().getServers(serverType);
    ASSERT(pModel->getConfiguration().getTotalActiveServers() > 1);

    ASSERT(serverRemoveInProgress == 0); // removing a server while another is being removed not supported yet

    serverRemoveInProgress++;
    BootComplete* pBootComplete = doRemoveServer(serverType);

    // cancel boot complete event if needed
    for (BootCompletes::iterator it = pendingMessages.begin(); it != pendingMessages.end(); ++it) {
        if ((*it)->getModuleId() == pBootComplete->getModuleId()) {
            cancelAndDelete(*it);
            pendingMessages.erase(it);
            break;
        }
    }
    delete pBootComplete;
}

void ExecutionManagerModBase::setBrownout(double factor) {
    Enter_Method("setBrownout()");
    cout << "t=" << simTime() << " executing setDimmer(" << 1.0 - factor << ")" << endl;
    pModel->setBrownoutFactor(factor);
    doSetBrownout(factor);
    emit(brownoutSetSignal, true);
}

void ExecutionManagerModBase::notifyRemoveServerCompleted(MTServerType::ServerType serverType) {

    pModel->removeServer(serverType);
    serverRemoveInProgress--;

    // emit signal to notify others (notably iProbe)
    emit(serverRemovedSignal, serverType);
}

void ExecutionManagerModBase::divertTraffic(LoadBalancer::TrafficLoad serverA,
        LoadBalancer::TrafficLoad serverB, LoadBalancer::TrafficLoad serverC) {
    Enter_Method("divertTraffic()");
    pModel->setTrafficLoad(serverA, serverB, serverC);
}

double ExecutionManagerModBase::getMeanAndVarianceFromParameter(const cPar& par, double& variance) const {
    if (par.isExpression()) {
        typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
        //std::cout << par.getExpression()->str() << std::endl;
        std::string serviceTimeDistribution = par.getExpression()->str();
        tokenizer tokens(serviceTimeDistribution, boost::char_separator<char>("(,)"));
        tokenizer::iterator it = tokens.begin();

        ASSERT(it != tokens.end());

        if (*it == "exponential") {
            ASSERT(++it != tokens.end());
            double mean = atof((*it).c_str());
            variance = pow(mean, 2);
            return mean;
        } else if (*it == "normal" || *it == "truncnormal") {
            ASSERT(++it != tokens.end());
            double mean = atof((*it).c_str());
            ASSERT(++it != tokens.end());
            variance = pow(atof((*it).c_str()), 2);
            return mean;
        }
        ASSERT(false); 
    } else {
        variance = 0;
        return par.doubleValue();
    }
    return 0;
}

MTServerType::ServerType ExecutionManagerModBase::getServerTypeFromName(const char* name) const {
    MTServerType::ServerType serverType = MTServerType::NONE;

    if (strncmp(name, "server_A_", 9) == 0) {
        serverType = MTServerType::A;
    } else if (strncmp(name, "server_B_", 9) == 0) {
        serverType = MTServerType::B;
    } else if (strncmp(name, "server_C_", 9) == 0) {
        serverType = MTServerType::C;
    } else {
        assert(false);
    }

    return serverType;
}
