// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_channel.h"

#include <app.h>
#include <feedback.h> // TODO add to build

#include "flutter/shell/platform/common/cpp/json_method_codec.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

static constexpr char kChannelName[] = "flutter/platform";

PlatformChannel::PlatformChannel(flutter::BinaryMessenger* messenger)
    : channel_(std::make_unique<flutter::MethodChannel<rapidjson::Document>>(
          messenger, kChannelName, &flutter::JsonMethodCodec::GetInstance())) {
  channel_->SetMethodCallHandler(
      [this](
          const flutter::MethodCall<rapidjson::Document>& call,
          std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result) {
        HandleMethodCall(call, std::move(result));
      });
}

PlatformChannel::~PlatformChannel() {}

namespace {

class HapticFeedbackManager {
public:
  HapticFeedbackManager() {
    // TODO: log
    auto ret = feedback_initialize();
    if (FEEDBACK_ERROR_NONE != ret) {
      //TODO log
      return;
    }

    bool supported = false;
    ret = feedback_is_supported_pattern(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_SIP, &supported);
    if (FEEDBACK_EROR_NONE != ret) {
      //TODO log
      return;
    }
    
    //TODO log supported
    properly_initialized_ = supported;
  }

  bool IsProperlyInitialized() {
    return properly_initialized_;
  }

  ~HapticFeedbackManager() {
    auto ret = feedback_deinitialize();
    if (FEEDBACK_ERROR_NONE != ret) {
      //TODO: log;
    }
    properly_initialized_ = false;
  }

  void Vibrate() {
    if (!properly_initialized_) {
      //TODO: log; throw? return an error code?
      return;
    }

    auto ret = feedback_play(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_SIP);
    if (FEEDBACK_ERROR_NONE != ret) {
      //TODO: log; throw?
      return;
    }
    //TODO: LOG
  }

private:
  bool properly_initialized_ = false;
};

} //  namespace


void PlatformChannel::HandleMethodCall(
    const flutter::MethodCall<rapidjson::Document>& call,
    std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result) {
  const auto method = call.method_name();

  if (method == "SystemNavigator.pop") {
    ui_app_exit();
    result->Success();
  } else if (method == "SystemSound.play") {
    result->NotImplemented();
  } else if (method == "HapticFeedback.vibrate") {
    result->NotImplemented();



  } else if (method == "Clipboard.getData") {
    result->NotImplemented();
  } else if (method == "Clipboard.setData") {
    result->NotImplemented();
  } else if (method == "Clipboard.hasStrings") {
    result->NotImplemented();
  } else if (method == "SystemChrome.setPreferredOrientations") {
    result->NotImplemented();
  } else if (method == "SystemChrome.setApplicationSwitcherDescription") {
    result->NotImplemented();
  } else if (method == "SystemChrome.setEnabledSystemUIOverlays") {
    result->NotImplemented();
  } else if (method == "SystemChrome.restoreSystemUIOverlays") {
    result->NotImplemented();
  } else if (method == "SystemChrome.setSystemUIOverlayStyle") {
    result->NotImplemented();
  } else {
    FT_LOGI("Unimplemented method: %s", method.c_str());
    result->NotImplemented();
  }
}
