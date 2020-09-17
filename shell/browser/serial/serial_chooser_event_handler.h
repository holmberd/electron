// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_EVENT_HANDLER_H_
#define SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_EVENT_HANDLER_H_

#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/serial/electron_serial_delegate.h"
#include "shell/browser/serial/serial_chooser_controller.h"

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace electron {

class ElectronSerialDelegate;
class SerialChooserController;

class SerialChooserEventHandler : public content::WebContentsObserver {
 public:
  SerialChooserEventHandler(
      std::unique_ptr<SerialChooserController> chooser_controller,
      content::WebContents* web_contents,
      base::WeakPtr<ElectronSerialDelegate> serial_delegate);
  ~SerialChooserEventHandler() override;

  void Close();
  base::OnceClosure MakeCloseClosure();
  void OnGetDevices(const std::vector<device::mojom::SerialPortInfoPtr>& ports);
  void OnPortAdded(const device::mojom::SerialPortInfoPtr& port);
  void OnPortRemoved(const device::mojom::SerialPortInfoPtr& port);

  // content::WebContentsObserver:
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;

 private:
  api::Session* GetSession();

  content::BrowserContext* browser_context_ = nullptr;

  std::unique_ptr<SerialChooserController> chooser_controller_;

  base::WeakPtr<ElectronSerialDelegate> serial_delegate_;

  base::WeakPtrFactory<SerialChooserEventHandler> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(SerialChooserEventHandler);
};

}  // namespace electron

#endif  // SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_EVENT_HANDLER_H_
