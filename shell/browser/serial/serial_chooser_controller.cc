// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/serial/serial_chooser_controller.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/serial/serial_chooser_context.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "ui/base/l10n/l10n_util.h"

namespace gin {

template <>
struct Converter<device::mojom::SerialPortInfoPtr> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const device::mojom::SerialPortInfoPtr& port) {
    gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
    dict.Set("portId", port->token.ToString());
    dict.Set("portName", port->path.BaseName().LossyDisplayName());
    if (port->display_name && !port->display_name->empty()) {
      dict.Set("displayName", *port->display_name);
    }
    if (port->persistent_id && !port->persistent_id->empty()) {
      dict.Set("persistentId", *port->persistent_id);
    }
    if (port->has_vendor_id) {
      dict.Set("vendorId", base::StringPrintf("%u", port->vendor_id));
    }
    if (port->has_product_id) {
      dict.Set("productId", base::StringPrintf("%u", port->product_id));
    }

    return gin::ConvertToV8(isolate, dict);
  }
};

}  // namespace gin

namespace electron {

SerialChooserController::SerialChooserController(
    content::RenderFrameHost* render_frame_host,
    std::vector<blink::mojom::SerialPortFilterPtr> filters,
    content::SerialChooser::Callback callback)
    : filters_(std::move(filters)), callback_(std::move(callback)) {
  api_web_contents_ = api::WebContents::From(
      content::WebContents::FromRenderFrameHost(render_frame_host));

  chooser_context_ = SerialChooserContext::GetInstance()->AsWeakPtr();
  DCHECK(chooser_context_);
  chooser_context_->GetPortManager()->GetDevices(base::BindOnce(
      &SerialChooserController::OnGetDevices, weak_factory_.GetWeakPtr()));
  observer_.Add(chooser_context_.get());
}

SerialChooserController::~SerialChooserController() {
  RunCallback(/*port=*/nullptr);
}

void SerialChooserController::OnPortAdded(
    const device::mojom::SerialPortInfo& port) {
  ports_.push_back(port.Clone());
  api_web_contents_->Emit("serial-port-added", port.Clone());
}

void SerialChooserController::OnPortRemoved(
    const device::mojom::SerialPortInfo& port) {
  const auto it = std::find_if(
      ports_.begin(), ports_.end(),
      [&port](const auto& ptr) { return ptr->token == port.token; });
  if (it != ports_.end()) {
    api_web_contents_->Emit("serial-port-removed", port.Clone());
    ports_.erase(it);
  }
}

void SerialChooserController::OnPortManagerConnectionError() {
  observer_.RemoveAll();
}

void SerialChooserController::OnDeviceChosen(const std::string& port_id) {
  if (port_id.empty()) {
    RunCallback(/*port=*/nullptr);
  } else {
    const auto it =
        std::find_if(ports_.begin(), ports_.end(), [&port_id](const auto& ptr) {
          return ptr->token.ToString() == port_id;
        });
    RunCallback(it->Clone());
  }
}

void SerialChooserController::OnGetDevices(
    std::vector<device::mojom::SerialPortInfoPtr> ports) {
  // Sort ports by file paths.
  std::sort(ports.begin(), ports.end(),
            [](const auto& port1, const auto& port2) {
              return port1->path.BaseName() < port2->path.BaseName();
            });

  for (auto& port : ports) {
    if (FilterMatchesAny(*port))
      ports_.push_back(std::move(port));
  }

  bool prevent_default = api_web_contents_->Emit(
      "select-serial-port", ports_,
      base::BindOnce(&SerialChooserController::OnDeviceChosen,
                     weak_factory_.GetWeakPtr()));
  if (!prevent_default) {
    RunCallback(/*port=*/nullptr);
  }
}

bool SerialChooserController::FilterMatchesAny(
    const device::mojom::SerialPortInfo& port) const {
  if (filters_.empty())
    return true;

  for (const auto& filter : filters_) {
    if (filter->has_vendor_id &&
        (!port.has_vendor_id || filter->vendor_id != port.vendor_id)) {
      continue;
    }
    if (filter->has_product_id &&
        (!port.has_product_id || filter->product_id != port.product_id)) {
      continue;
    }
    return true;
  }

  return false;
}

void SerialChooserController::RunCallback(
    device::mojom::SerialPortInfoPtr port) {
  if (callback_) {
    std::move(callback_).Run(std::move(port));
  }
}

}  // namespace electron
