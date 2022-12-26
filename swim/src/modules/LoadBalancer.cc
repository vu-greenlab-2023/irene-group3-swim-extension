//
 // Copyright (c) 2015 Carnegie Mellon University. All Rights Reserved.

 // Redistribution and use in source and binary forms, with or without
 // modification, are permitted provided that the following conditions
 // are met:

 // 1. Redistributions of source code must retain the above copyright
 // notice, this list of conditions and the following acknowledgments
 // and disclaimers.

 // 2. Redistributions in binary form must reproduce the above
 // copyright notice, this list of conditions and the following
 // disclaimer in the documentation and/or other materials provided
 // with the distribution.

 // 3. The names "Carnegie Mellon University," "SEI" and/or "Software
 // Engineering Institute" shall not be used to endorse or promote
 // products derived from this software without prior written
 // permission. For written permission, please contact
 // permission@sei.cmu.edu.

 // 4. Products derived from this software may not be called "SEI" nor
 // may "SEI" appear in their names without prior written permission of
 // permission@sei.cmu.edu.

 // 5. Redistributions of any form whatsoever must retain the following
 // acknowledgment:

 // This material is based upon work funded and supported by the
 // Department of Defense under Contract No. FA8721-05-C-0003 with
 // Carnegie Mellon University for the operation of the Software
 // Engineering Institute, a federally funded research and development
 // center.

 // Any opinions, findings and conclusions or recommendations expressed
 // in this material are those of the author(s) and do not necessarily
 // reflect the views of the United States Department of Defense.

 // NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE
 // ENGINEERING INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS"
 // BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND,
 // EITHER EXPRESSED OR IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT
 // LIMITED TO, WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY,
 // EXCLUSIVITY, OR RESULTS OBTAINED FROM USE OF THE MATERIAL. CARNEGIE
 // MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH
 // RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT
 // INFRINGEMENT.

 // This material has been approved for public release and unlimited
 // distribution.

 // DM-0002494
 //

#include "LoadBalancer.h"
#include <model/Model.h>
#include <MTServerType.h>
#include <assert.h>

Define_Module(LoadBalancer);

void LoadBalancer::initialize()
{
    const char *algName = par("routingAlgorithm");
    if (strcmp(algName, "random") == 0) {
        routingAlgorithm = ALG_RANDOM;
    } else if (strcmp(algName, "roundRobin") == 0) {
        routingAlgorithm = ALG_ROUND_ROBIN;
    } else if (strcmp(algName, "probDist") == 0) {
        routingAlgorithm = ALG_PROB_DIST;
    }

    rrCounter = -1;
}

void LoadBalancer::handleMessage(cMessage *msg)
{
    int outGateIndex = -1;  // by default we drop the message
    //getOutIndex();
    switch (routingAlgorithm)
    {
        case ALG_RANDOM:
            outGateIndex = par("randomGateIndex");
            break;
        case ALG_ROUND_ROBIN:
            rrCounter = (rrCounter + 1) % gateSize("out");
            outGateIndex = rrCounter;
            break;
        case ALG_PROB_DIST:
            outGateIndex = getOutIndex();
            break;
        default:
            outGateIndex = -1;
            break;
    }

    // send out if the index is legal
    if (outGateIndex < 0 || outGateIndex >= gateSize("out"))
        error("Invalid output gate selected during routing");

    send(msg, "out", outGateIndex);
}

int LoadBalancer::getOutIndex() {
    bool debug = false;
    int RNG = 4;

    int outGateIndex = -1;
    Model* model = check_and_cast<Model*> (getParentModule()->getSubmodule("model"));

    float serverA = float(model->getConfiguration().getTraffic(MTServerType::ServerType::POWERFUL)*25)/100;
    float serverB = float(model->getConfiguration().getTraffic(MTServerType::ServerType::AVERAGE)*25)/100;

    double u = uniform(0, 1, RNG);

    for (cModule::GateIterator i(this); !i.end(); i++) {
        cGate* gate = i();

        if (debug) {
            std::cout << gate->getFullName() << ": ";
            std::cout << "id = " << gate->getId() << ", ";
            if (!gate->isVector()) {
                std::cout << "scalar gate, ";
            } else {
                std::cout << "gate " << gate->getIndex() << " in vector "
                        << gate->getName() << " of size "
                        << gate->getVectorSize() << ", ";
            }
            std::cout << "type:" << cGate::getTypeName(gate->getType()) << "## " << gate->getIndex();
            std::cout << "\n";
        }

        if (gate->getType() == cGate::OUTPUT) {
            cGate* connectedGate = gate->getNextGate();
            if (connectedGate != NULL) {
                cModule* mod = connectedGate->getOwnerModule();

                if (debug) std::cout << mod->getFullName() << "    u = " << u << endl;

                if ((strncmp(mod->getFullName(), "server_A_", 9) == 0 && u <= serverA)
                        || ((strncmp(mod->getFullName(), "server_B_", 9) == 0 && u > serverA && u <= (serverA + serverB)))
                        || (strncmp(mod->getFullName(), "server_C_", 9) == 0 && u > (serverA + serverB))) {
                    outGateIndex = gate->getIndex();
                    break;
                }
            }
        }
    }

    if (debug) std::cout << outGateIndex << std::endl;

    return outGateIndex;
}
