//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html
#include "cleavageTestApp.h"
#include "cleavageApp.h"
#include "Moose.h"
#include "AppFactory.h"
#include "MooseSyntax.h"

InputParameters
cleavageTestApp::validParams()
{
  InputParameters params = cleavageApp::validParams();
  params.set<bool>("use_legacy_material_output") = false;
  params.set<bool>("use_legacy_initial_residual_evaluation_behavior") = false;
  return params;
}

cleavageTestApp::cleavageTestApp(InputParameters parameters) : MooseApp(parameters)
{
  cleavageTestApp::registerAll(
      _factory, _action_factory, _syntax, getParam<bool>("allow_test_objects"));
}

cleavageTestApp::~cleavageTestApp() {}

void
cleavageTestApp::registerAll(Factory & f, ActionFactory & af, Syntax & s, bool use_test_objs)
{
  cleavageApp::registerAll(f, af, s);
  if (use_test_objs)
  {
    Registry::registerObjectsTo(f, {"cleavageTestApp"});
    Registry::registerActionsTo(af, {"cleavageTestApp"});
  }
}

void
cleavageTestApp::registerApps()
{
  registerApp(cleavageApp);
  registerApp(cleavageTestApp);
}

/***************************************************************************************************
 *********************** Dynamic Library Entry Points - DO NOT MODIFY ******************************
 **************************************************************************************************/
// External entry point for dynamic application loading
extern "C" void
cleavageTestApp__registerAll(Factory & f, ActionFactory & af, Syntax & s)
{
  cleavageTestApp::registerAll(f, af, s);
}
extern "C" void
cleavageTestApp__registerApps()
{
  cleavageTestApp::registerApps();
}
