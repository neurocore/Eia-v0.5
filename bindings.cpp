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
#include <pybind11/native_enum.h>
#include <pybind11/stl.h>
#include "tuning.h"

namespace py = pybind11;
using namespace eia;
using namespace std;

unique_ptr<Tuner> g_tuner;

enum class TunerType { Static, PST };
enum class LossType { MSE, BCE };

void load(string file, int batch_sz
  , TunerType tuner_type = TunerType::Static
  , LossType loss_type = LossType::MSE)
{
  unique_ptr<Loss> loss;
  switch (loss_type)
  {
    case LossType::MSE:
      loss = make_unique<MSE>();
      break;

    case LossType::BCE:
      loss = make_unique<BCE>();
      break;
  }

  switch (tuner_type)
  {
    case TunerType::Static:
      g_tuner = make_unique<TunerStatic>(std::move(loss), batch_sz);
      break;

    case TunerType::PST:
      g_tuner = make_unique<TunerPST>(std::move(loss), batch_sz);
      break;
  }

  g_tuner->open(file);

  log("Positions: {}\n", g_tuner->size());
  report_num_threads();
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

  // Defining enums

  py::native_enum<TunerType>(m, "TunerType", "enum.Enum")
    .value("Static", TunerType::Static)
    .value("PST"   , TunerType::PST)
    .export_values()
    .finalize();

  py::native_enum<LossType>(m, "LossType", "enum.Enum")
    .value("MSE", LossType::MSE)
    .value("BCE", LossType::BCE)
    .export_values()
    .finalize();

  // Defining functions

  m.def("load",  &load, "Load dataset with given batch size"
    , py::arg("file"), py::arg("batch_size")
    , py::arg("tuner_type") = TunerType::Static
    , py::arg("loss_type") = LossType::MSE);
  m.def("base",  &base,  "Get the basis tune");
  m.def("score", &score, "Loss score for tune");
}
#endif
