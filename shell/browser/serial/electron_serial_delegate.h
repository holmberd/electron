// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_SERIAL_ELECTRON_SERIAL_DELEGATE_H_
#define SHELL_BROWSER_SERIAL_ELECTRON_SERIAL_DELEGATE_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/serial_delegate.h"
#include "shell/browser/serial/serial_chooser_controller.h"
#include "shell/browser/serial/serial_chooser_event_handler.h"

namespace electron {

class SerialChooserController;
class SerialChooserEventHandler;

class ElectronSerialDelegate : public content::SerialDelegate {
 public:
  ElectronSerialDelegate();
  ~ElectronSerialDelegate() override;

  std::unique_ptr<content::SerialChooser> RunChooser(
      content::RenderFrameHost* frame,
      std::vector<blink::mojom::SerialPortFilterPtr> filters,
      content::SerialChooser::Callback callback) override;
  bool CanRequestPortPermission(content::RenderFrameHost* frame) override;
  bool HasPortPermission(content::RenderFrameHost* frame,
                         const device::mojom::SerialPortInfo& port) override;
  device::mojom::SerialPortManager* GetPortManager(
      content::RenderFrameHost* frame) override;
  void AddObserver(content::RenderFrameHost* frame,
                   Observer* observer) override;
  void RemoveObserver(content::RenderFrameHost* frame,
                      Observer* observer) override;

  void DeleteHandlerForFrame(content::RenderFrameHost* render_frame_host);

 private:
  SerialChooserEventHandler* HandlerForFrame(
      content::RenderFrameHost* render_frame_host);
  SerialChooserEventHandler* AddHandlerForFrame(
      content::RenderFrameHost* render_frame_host,
      std::unique_ptr<SerialChooserController> chooser_controller);

  std::unordered_map<content::RenderFrameHost*,
                     std::unique_ptr<SerialChooserEventHandler>>
      event_handler_map_;

  base::WeakPtrFactory<ElectronSerialDelegate> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ElectronSerialDelegate);
};

}  // namespace electron

#endif  // SHELL_BROWSER_SERIAL_ELECTRON_SERIAL_DELEGATE_H_
