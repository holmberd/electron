// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/serial/serial_chooser_controller.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/serial/serial_chooser_context.h"
#include "shell/browser/serial/serial_chooser_context_factory.h"

namespace electron {

SerialChooserController::SerialChooserController(
    content::RenderFrameHost* render_frame_host,
    std::vector<blink::mojom::SerialPortFilterPtr> filters,
    content::SerialChooser::Callback callback)
    : filters_(std::move(filters)), callback_(std::move(callback)) {
  LOG(INFO) << "In SerialChooserController::SerialChooserController";
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  requesting_origin_ = render_frame_host->GetLastCommittedOrigin();
  embedding_origin_ = web_contents->GetMainFrame()->GetLastCommittedOrigin();

  browser_context_ = web_contents->GetBrowserContext();
  chooser_context_ =
      SerialChooserContextFactory::GetForBrowserContext(browser_context_)
          ->AsWeakPtr();
  DCHECK(chooser_context_);
  chooser_context_->GetPortManager()->GetDevices(base::BindOnce(
      &SerialChooserController::OnGetDevices, weak_factory_.GetWeakPtr()));
  observer_.Add(chooser_context_.get());
}

SerialChooserController::~SerialChooserController() {
  LOG(INFO) << "In SerialChooserController::~SerialChooserController";
  RunCallback(/*port=*/nullptr);
}

void SerialChooserController::OnPortAdded(
    const device::mojom::SerialPortInfo& port) {
  ports_.push_back(port.Clone());
  if (event_handler_) {
    event_handler_->OnPortAdded(port.Clone());
  }
}

void SerialChooserController::OnPortRemoved(
    const device::mojom::SerialPortInfo& port) {
  const auto it = std::find_if(
      ports_.begin(), ports_.end(),
      [&port](const auto& ptr) { return ptr->token == port.token; });
  if (it != ports_.end()) {
    if (event_handler_) {
      event_handler_->OnPortRemoved(port.Clone());
    }
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
    chooser_context_->GrantPortPermission(requesting_origin_, embedding_origin_,
                                          *it->get());
    RunCallback(it->Clone());
  }
}

void SerialChooserController::OnGetDevices(
    std::vector<device::mojom::SerialPortInfoPtr> ports) {
  LOG(INFO) << "In SerialChooserController::OnGetDevices";
  // Sort ports by file paths.
  std::sort(ports.begin(), ports.end(),
            [](const auto& port1, const auto& port2) {
              return port1->path.BaseName() < port2->path.BaseName();
            });

  for (auto& port : ports) {
    if (FilterMatchesAny(*port))
      ports_.push_back(std::move(port));
  }

  if (event_handler_) {
    event_handler_->SerialChooserEventHandler::OnGetDevices(ports_);
  } else {
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
