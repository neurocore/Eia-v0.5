#ifdef PY_BINDINGS

// Engine implements python3 bindings for automated
//  eval tuning (such as CMA-ES, PSO or any other)
//  via interop library 'pybind11'
// 
// Note that in this case we are building shared
//  library to use it in python (dll)
//
// Don't forget to include paths to 'pybind11'
//  and 'python.h' and path to 'python.lib'
//
// Also, resulting eia_tuner.dll should be renamed
//  to 'pyd' extension and placed in python/dlls

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "tuning.h"

namespace py = pybind11;
using namespace eia;
using namespace std;

unique_ptr<TunerStatic> g_tuner;

void load(string file, int batch_sz)
{
  auto loss = make_unique<MSE>();
  g_tuner = make_unique<TunerStatic>(std::move(loss), batch_sz);
  g_tuner->open(file);

  log("Positions: {}\n", g_tuner->size());
}

Tune base()
{
  return g_tuner ? g_tuner->get_init() : Tune{};
}

double score(Tune v)
{
  return g_tuner ? g_tuner->score(v).loss : 0.;
}

PYBIND11_MODULE(eia_tuner, m)
{
  m.doc() = "pybind11 plugin for Eia v0.5 chess engine eval tuning";
  m.def("load",  &load,  "Load dataset with given batch size");
  m.def("base",  &base,  "Get the basis tune");
  m.def("score", &score, "Loss score for tune");
}
#endif
