// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_channel.h"

#include <app.h>
#include <feedback.h>

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

#if defined(MOBILE_PROFILE) || defined(WEARABLE_PROFILE)
namespace {

class FeedbackManager {
 public:
  enum class ResultCode {
    kOk,
    kNotSupportedError,
    kPermissionDeniedError,
    kUnknownError
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
      return ResultCode::kUnknownError;
    }

    if (!permission_granted_) {
      FT_LOGD("Cannot run Vibrate(): permission denied");
      return ResultCode::kPermissionDeniedError;
    }

    if (!vibration_supported_) {
      FT_LOGD("HapticFeedback.Vibrate() is not supported");
      return ResultCode::kNotSupportedError;
    }

    auto ret =
        feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_SIP);
    if (FEEDBACK_ERROR_NONE == ret) {
      FT_LOGD("feedback_play_type() succeeded");
      return ResultCode::kOk;
    }

    FT_LOGD("feedback_play_type() failed with error: [%d] (%s)", ret,
            get_error_message(ret));

    if (FEEDBACK_ERROR_PERMISSION_DENIED == ret) {
      permission_granted_ = false;
      return ResultCode::kPermissionDeniedError;
    } else if (FEEDBACK_ERROR_NOT_SUPPORTED) {
      return ResultCode::kNotSupportedError;
    } else {
      return ResultCode::kUnknownError;
    }
  }

  static std::string GetErrorMessage(const std::string& method_name,
                                     ResultCode result_code) {
    FT_LOGD(
        "Enter FeedbackManager::GetErrorMessage(): method_name: (%s), "
        "result_code: [%d]",
        method_name.c_str(), static_cast<int>(result_code));

    switch (result_code) {
      case ResultCode::kNotSupportedError:
        return method_name + "() is not supported";
      case ResultCode::kPermissionDeniedError:
        return std::string{"No permission to run "} + method_name +
               "(). Add "
               "\"http://tizen.org/privilege/feedback\" privilege to "
               "tizen-manifest.xml "
               "to use this method";
      case ResultCode::kUnknownError:
      default:
        return std::string{"An unknown error on "} + method_name + "()";
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
    if (FEEDBACK_ERROR_NONE == ret) {
      FT_LOGD("feedback_is_supported_pattern() succeeded");
      return;
    }

    FT_LOGD("feedback_is_supported_pattern() failed with error: [%d] (%s)", ret,
            get_error_message(ret));
    if (FEEDBACK_ERROR_PERMISSION_DENIED == ret) {
      permission_granted_ = false;
    } else if (FEEDBACK_ERROR_NONE != ret) {
      return;
    }
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
  /*
   * We need this flag to differentiate between feedback_is_supported_pattern()
   * failure due to a missing privilege and other causes.
   */
  bool permission_granted_ = true;
  bool vibration_supported_ = false;
};

}  //  namespace
#endif  // defined(MOBILE_PROFILE) || defined(WEARABLE_PROFILE)

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

    const std::string error_message = "Could not vibrate";

    const auto vibrate_variant_name = call.arguments()->GetString();

#if defined(MOBILE_PROFILE) || defined(WEARABLE_PROFILE)
    /*
     * We use a single type of vibration (FEEDBACK_PATTERN_SIP) to implement
     * HapticFeedback's vibrate, lightImpact, mediumImpact, heavyImpact
     * and selectionClick methods, because Tizen's "feedback" module
     * has no dedicated vibration types for them.
     * Thus, we ignore the "arguments" contents for "HapticFeedback.vibrate"
     * calls.
     */

    auto ret = FeedbackManager::GetInstance().Vibrate();
    if (FeedbackManager::ResultCode::kOk == ret) {
      result->Success();
      return;
    }

    const auto error_cause =
        FeedbackManager::GetErrorMessage(vibrate_variant_name, ret);
    FT_LOGE("%s: %s", error_cause.c_str(), error_message.c_str());
    result->Error(error_cause, error_message);
#else
    const auto error_cause =
        std::string{vibrate_variant_name} + "() is not supported";
    result->Error(error_cause.c_str(), error_message.c_str());
#endif  // defined(MOBILE_PROFILE) || defined(WEARABLE_PROFILE)
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
