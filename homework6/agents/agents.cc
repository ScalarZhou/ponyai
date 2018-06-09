// Copyright @2018 Pony AI Inc. All rights reserved.

#include "homework6/agents/agents.h"

#include "homework6/agents/sample/sample_agent.h"
//#include "homework6/agents/hqztrue/hqztrue_agent.h"

// Register sample vehicle agent to a factory with its type name "sample_agent"
static simulation::Registrar<::sample::SampleVehicleAgent> registrar("sample_agent");
//static simulation::Registrar<::hqztrue::FrogVehicleAgent> registrar("hqztrue_agent");


