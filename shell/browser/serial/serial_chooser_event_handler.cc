// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/serial/serial_chooser_event_handler.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/serial/serial_chooser_context.h"
#include "shell/browser/serial/serial_chooser_context_factory.h"
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

SerialChooserEventHandler::SerialChooserEventHandler(
    std::unique_ptr<SerialChooserController> chooser_controller,
    content::WebContents* web_contents,
    base::WeakPtr<ElectronSerialDelegate> serial_delegate)
    : WebContentsObserver(web_contents),
      chooser_controller_(std::move(chooser_controller)),
      serial_delegate_(serial_delegate) {
  chooser_controller_->SetEventHandler(this);
  browser_context_ = web_contents->GetBrowserContext();
}

SerialChooserEventHandler::~SerialChooserEventHandler() {
  LOG(INFO) << "DESTROYING SerialChooserEventHandler";
}

api::Session* SerialChooserEventHandler::GetSession() {
  if (!browser_context_) {
    return nullptr;
  }
  return api::Session::FromBrowserContext(browser_context_);
}

void SerialChooserEventHandler::OnPortAdded(
    const device::mojom::SerialPortInfoPtr& port) {
  auto* session = GetSession();
  if (session) {
    session->Emit("serial-port-added", port);
  }
}

void SerialChooserEventHandler::OnPortRemoved(
    const device::mojom::SerialPortInfoPtr& port) {
  auto* session = GetSession();
  if (session) {
    session->Emit("serial-port-removed", port);
  }
}

void SerialChooserEventHandler::OnGetDevices(
    const std::vector<device::mojom::SerialPortInfoPtr>& ports) {
  bool prevent_default = false;
  auto* session = GetSession();
  if (session) {
    prevent_default =
        session->Emit("select-serial-port", ports,
                      base::AdaptCallbackForRepeating(base::BindOnce(
                          &SerialChooserController::OnDeviceChosen,
                          chooser_controller_->GetWeakPtr())));
  }
  if (!prevent_default && chooser_controller_) {
    chooser_controller_->RunCallback(/*port=*/nullptr);
  }
}

base::OnceClosure SerialChooserEventHandler::MakeCloseClosure() {
  return base::BindOnce(&SerialChooserEventHandler::Close,
                        weak_factory_.GetWeakPtr());
}

void SerialChooserEventHandler::Close() {
  auto* session = GetSession();
  if (session) {
    session->Emit("select-serial-port-cancelled");
  }
}

void SerialChooserEventHandler::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  LOG(INFO) << "In SerialChooserEventHandler::RenderFrameDeleted";
  if (serial_delegate_) {
    serial_delegate_->DeleteHandlerForFrame(render_frame_host);
  }
}

}  // namespace electron
