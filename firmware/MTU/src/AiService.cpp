#include "AiService.h"
#include "Config.h"
#include <cstring>

// ============== Edge Impulse Integration ==============
//
// After training on Edge Impulse Studio and exporting the Arduino library:
//   1. Extract the ZIP into  lib/edge_impulse/
//   2. Set ENABLE_EDGE_IMPULSE to 1 in Config.h
//   3. Change the include below to match your project name:
//        #include <your_project_inference.h>
//   4. Build and upload.
//
// The data forwarder mode (ENABLE_DATA_FORWARDER) outputs sensor readings
// as CSV on Serial so the edge-impulse-data-forwarder CLI can capture them
// for training.  That mode and the inference mode are mutually exclusive.

#if ENABLE_EDGE_IMPULSE
// ── Uncomment and change to your exported project name ──
// #include <your_project_inference.h>
#endif

void AiService::begin()
{
#if ENABLE_EDGE_IMPULSE
  // Edge Impulse classifier is stateless — nothing to initialize.
  // If the model uses a DSP block that needs warmup, add it here.
#endif
}

AiResult AiService::run(const TelemetrySnapshot &snapshot)
{
  AiResult result;
  result.available = true;
  result.timestamp = millis();

#if ENABLE_EDGE_IMPULSE
  // ────────────────────────────────────────────────────────
  // Real Edge Impulse inference.
  //
  // Prerequisites:
  //   - The exported Arduino library is in  lib/edge_impulse/
  //   - The #include above is uncommented and matches your project
  //   - EI_FEATURES_COUNT in Config.h matches the model input
  //
  // Uncomment the block below once the library is in place:
  // ────────────────────────────────────────────────────────

  /*
  float features[EI_FEATURES_COUNT] = {
      snapshot.turb1.value, snapshot.turb2.value,
      snapshot.ph1.value, snapshot.ph2.value,
      snapshot.flow1.value, snapshot.flow2.value,
      snapshot.pressure1.value, snapshot.pressure2.value,
      snapshot.temp1.value, snapshot.temp2.value,
      snapshot.pumpCurrent.value
  };

  signal_t signal;
  int err_sig = numpy::signal_from_buffer(features, EI_FEATURES_COUNT, &signal);
  if (err_sig != 0) {
    strncpy(result.label, "sig_error", sizeof(result.label) - 1);
    result.confidence = 0.0f;
    result.fault = true;
    result.label[sizeof(result.label) - 1] = '\0';
    return result;
  }

  ei_impulse_result_t ei_result = {0};
  EI_IMPULSE_ERROR ei_err = run_classifier(&signal, &ei_result, false);

  if (ei_err == EI_IMPULSE_OK) {
    // Find classification with highest confidence
    float maxConf = 0.0f;
    int maxIdx = 0;
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
      if (ei_result.classification[i].value > maxConf) {
        maxConf = ei_result.classification[i].value;
        maxIdx = i;
      }
    }
    strncpy(result.label, ei_result.classification[maxIdx].label,
            sizeof(result.label) - 1);
    result.confidence = maxConf;
    result.fault = (strcmp(result.label, "normal") != 0);
  } else {
    strncpy(result.label, "ei_error", sizeof(result.label) - 1);
    result.confidence = 0.0f;
    result.fault = true;
  }
  */

  // Placeholder until the model library is placed in lib/edge_impulse/
  strncpy(result.label, "ei_pending", sizeof(result.label) - 1);
  result.confidence = 0.0f;
  result.fault = false;

#else
  // ────────────────────────────────────────────────────────
  // Lightweight rule-based placeholder for demos before the
  // real model exists. Useful for testing the dashboard and
  // control flow without Edge Impulse.
  // ────────────────────────────────────────────────────────

  if (snapshot.error != ERR_NONE)
  {
    strncpy(result.label, errorToString(snapshot.error), sizeof(result.label) - 1);
    result.confidence = 0.95f;
    result.fault = true;
  }
  else if (snapshot.turb1.value > (MAX_TURBIDITY * 0.85f) || snapshot.turb2.value > (MAX_TURBIDITY * 0.85f))
  {
    strncpy(result.label, "turbidity_warn", sizeof(result.label) - 1);
    result.confidence = 0.75f;
    result.fault = false;
  }
  else if (snapshot.ph1.value > (MAX_PH * 0.95f) || snapshot.ph1.value < (MIN_PH * 1.05f))
  {
    strncpy(result.label, "ph_warning", sizeof(result.label) - 1);
    result.confidence = 0.75f;
    result.fault = false;
  }
  else if (snapshot.pressure1.value > (MAX_PRESSURE * 0.85f))
  {
    strncpy(result.label, "pressure_warn", sizeof(result.label) - 1);
    result.confidence = 0.75f;
    result.fault = false;
  }
  else if (snapshot.temp1.value > (MAX_TEMPERATURE * 0.90f))
  {
    strncpy(result.label, "temp_warning", sizeof(result.label) - 1);
    result.confidence = 0.75f;
    result.fault = false;
  }
  else if (snapshot.pumpCurrent.isValid && snapshot.pumpCurrent.value > (MAX_PUMP_CURRENT * 0.85f))
  {
    strncpy(result.label, "pump_current_warn", sizeof(result.label) - 1);
    result.confidence = 0.80f;
    result.fault = false;
  }
  else
  {
    strncpy(result.label, "normal", sizeof(result.label) - 1);
    result.confidence = 0.80f;
    result.fault = false;
  }
#endif

  result.label[sizeof(result.label) - 1] = '\0';
  return result;
}
