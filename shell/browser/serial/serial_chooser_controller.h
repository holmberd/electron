// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTROLLER_H_
#define SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "content/public/browser/serial_chooser.h"
#include "content/public/browser/web_contents.h"
#include "services/device/public/mojom/serial.mojom-forward.h"
#include "shell/browser/serial/serial_chooser_context.h"
#include "shell/browser/serial/serial_chooser_event_handler.h"
#include "third_party/blink/public/mojom/serial/serial.mojom.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace electron {

class SerialChooserEventHandler;

// SerialChooserController provides data for the Serial API permission prompt.
class SerialChooserController final
    : public SerialChooserContext::PortObserver {
 public:
  SerialChooserController(
      content::RenderFrameHost* render_frame_host,
      std::vector<blink::mojom::SerialPortFilterPtr> filters,
      content::SerialChooser::Callback callback);
  ~SerialChooserController() override;

  // SerialChooserContext::PortObserver:
  void OnPortAdded(const device::mojom::SerialPortInfo& port) override;
  void OnPortRemoved(const device::mojom::SerialPortInfo& port) override;
  void OnPortManagerConnectionError() override;

  void SetEventHandler(SerialChooserEventHandler* event_handler) {
    event_handler_ = event_handler;
  }
  void OnDeviceChosen(const std::string& port_id);
  void RunCallback(device::mojom::SerialPortInfoPtr port);

  base::WeakPtr<SerialChooserController> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  void OnGetDevices(std::vector<device::mojom::SerialPortInfoPtr> ports);
  bool FilterMatchesAny(const device::mojom::SerialPortInfo& port) const;

  std::vector<blink::mojom::SerialPortFilterPtr> filters_;
  content::SerialChooser::Callback callback_;
  url::Origin requesting_origin_;
  url::Origin embedding_origin_;

  base::WeakPtr<SerialChooserContext> chooser_context_;
  ScopedObserver<SerialChooserContext,
                 SerialChooserContext::PortObserver,
                 &SerialChooserContext::AddPortObserver,
                 &SerialChooserContext::RemovePortObserver>
      observer_{this};

  content::BrowserContext* browser_context_ = nullptr;

  std::vector<device::mojom::SerialPortInfoPtr> ports_;

  SerialChooserEventHandler* event_handler_ = nullptr;

  base::WeakPtrFactory<SerialChooserController> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(SerialChooserController);
};

}  // namespace electron

#endif  // SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTROLLER_H_
