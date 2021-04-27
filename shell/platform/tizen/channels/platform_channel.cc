// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_channel.h"

#include <app.h>
#include <feedback.h>

#include "flutter/shell/platform/common/cpp/json_method_codec.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

static constexpr char kChannelName[] = "flutter/platform";

#if defined(MOBILE) || defined(WEARABLE)
static constexpr char kUnsupportedHapticFeedbackError[] =
    "HapticFeedback.vibrate() is not supported";
static constexpr char kPermissionDeniedHapticFeedbackError[] =
    "No permission to run HapticFeedback.vibrate(). Add "
    "\"http://tizen.org/privilege/feedback\" privilege to tizen-manifest.xml "
    "to use this method";
static constexpr char kUnknownHapticFeedbackError[] =
    "An unknown error on HapticFeedback.vibrate()";
#endif

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

#if defined(MOBILE) || defined(WEARABLE)
namespace {

class FeedbackManager {
 public:
  enum class ResultCode {
    OK,
    NOT_SUPPORTED_ERROR,
    PERMISSION_DENIED_ERROR,
    UNKNOWN_ERROR
  };

  static FeedbackManager& GetInstance() {
    FT_LOGD("Enter FeedbackManager::GetInstance()");

    static FeedbackManager instance;
    return instance;
  }

  FeedbackManager(const FeedbackManager&) = delete;
  FeedbackManager& operator=(const FeedbackManager&) = delete;

  ResultCode Vibrate() {
    FT_LOGD("Enter FeedbackManager::Vibrate()");

    if (!properly_initialized_) {
      FT_LOGD(
          "Cannot run Vibrate(): FeedbackManager.properly_initialized_ is "
          "false");
      return ResultCode::UNKNOWN_ERROR;
    }

    if (!vibration_supported_) {
      FT_LOGD("HapticFeedback.Vibrate() is not supported");
      return ResultCode::NOT_SUPPORTED_ERROR;
    }

    auto ret =
        feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_SIP);
    FT_LOGD("feedback_play_type() returned: [%d] (%s)", ret,
            get_error_message(ret));
    if (FEEDBACK_ERROR_NONE == ret) {
      return ResultCode::OK;
    } else if (FEEDBACK_ERROR_PERMISSION_DENIED == ret) {
      return ResultCode::UNKNOWN_ERROR;
    } else if (FEEDBACK_ERROR_NOT_SUPPORTED) {
      return ResultCode::NOT_SUPPORTED_ERROR;
    } else {
      return ResultCode::UNKNOWN_ERROR;
    }
  }

 private:
  FeedbackManager() {
    FT_LOGD("Enter FeedbackManager::FeedbackManager()");

    auto ret = feedback_initialize();
    if (FEEDBACK_ERROR_NONE != ret) {
      FT_LOGD("feedback_initialize() failed with error: [%d] (%s)", ret,
              get_error_message(ret));
      return;
    }
    FT_LOGD("feedback_initialize() succeeded");
    properly_initialized_ = true;

    ret = feedback_is_supported_pattern(
        FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_SIP, &vibration_supported_);
    if (FEEDBACK_ERROR_NONE != ret) {
      FT_LOGD("feedback_is_supported_pattern() failed with error: [%d] (%s)",
              ret, get_error_message(ret));
      return;
    }
    FT_LOGD("feedback_is_supported_pattern() succeeded");
  }

  ~FeedbackManager() {
    FT_LOGD("Enter FeedbackManager::~FeedbackManager");

    if (!properly_initialized_) {
      return;
    }

    auto ret = feedback_deinitialize();
    if (FEEDBACK_ERROR_NONE != ret) {
      FT_LOGD("feedback_deinitialize() failed with error: [%d] (%s)", ret,
              get_error_message(ret));
      return;
    }
    FT_LOGD("feedback_deinitialize() succeeded");
  }

  bool properly_initialized_ = false;
  bool vibration_supported_ = false;
};

}  //  namespace
#endif

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
    FT_LOGD("HapticFeedback.vibrate() call received");

#if defined(MOBILE) || defined(WEARABLE)
    auto ret = FeedbackManager::GetInstance().Vibrate();
    if (FeedbackManager::ResultCode::OK == ret) {
      result->Success();
      return;
    }

    const std::string error_message = "Could not vibrate";
    if (FeedbackManager::ResultCode::NOT_SUPPORTED_ERROR == ret) {
      result->Error(kUnsupportedHapticFeedbackError, error_message);
    } else if (FeedbackManager::ResultCode::PERMISSION_DENIED_ERROR == ret) {
      result->Error(kPermissionDeniedHapticFeedbackError, error_message);
    } else {
      result->Error(kUnknownHapticFeedbackError, error_message);
    }
#else
    result->NotImplemented();
#endif
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
