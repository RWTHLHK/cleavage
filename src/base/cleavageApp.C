#include "cleavageApp.h"
#include "Moose.h"
#include "AppFactory.h"
#include "ModulesApp.h"
#include "MooseSyntax.h"

InputParameters
cleavageApp::validParams()
{
  InputParameters params = MooseApp::validParams();
  params.set<bool>("use_legacy_material_output") = false;
  params.set<bool>("use_legacy_initial_residual_evaluation_behavior") = false;
  return params;
}

cleavageApp::cleavageApp(InputParameters parameters) : MooseApp(parameters)
{
  cleavageApp::registerAll(_factory, _action_factory, _syntax);
}

cleavageApp::~cleavageApp() {}

void
cleavageApp::registerAll(Factory & f, ActionFactory & af, Syntax & s)
{
  ModulesApp::registerAllObjects<cleavageApp>(f, af, s);
  Registry::registerObjectsTo(f, {"cleavageApp"});
  Registry::registerActionsTo(af, {"cleavageApp"});

  /* register custom execute flags, action syntax, etc. here */
}

void
cleavageApp::registerApps()
{
  registerApp(cleavageApp);
}

/***************************************************************************************************
 *********************** Dynamic Library Entry Points - DO NOT MODIFY ******************************
 **************************************************************************************************/
extern "C" void
cleavageApp__registerAll(Factory & f, ActionFactory & af, Syntax & s)
{
  cleavageApp::registerAll(f, af, s);
}
extern "C" void
cleavageApp__registerApps()
{
  cleavageApp::registerApps();
}
