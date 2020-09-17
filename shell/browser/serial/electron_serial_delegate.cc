// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/serial/electron_serial_delegate.h"

#include <utility>

#include "content/public/browser/web_contents.h"
#include "shell/browser/serial/serial_chooser.h"
#include "shell/browser/serial/serial_chooser_context.h"
#include "shell/browser/serial/serial_chooser_context_factory.h"
#include "shell/browser/serial/serial_chooser_controller.h"
#include "shell/browser/serial/serial_chooser_event_handler.h"

namespace electron {

SerialChooserContext* GetChooserContext(content::RenderFrameHost* frame) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(frame);
  auto* browser_context = web_contents->GetBrowserContext();
  return SerialChooserContextFactory::GetForBrowserContext(browser_context);
}

ElectronSerialDelegate::ElectronSerialDelegate() {
  LOG(INFO) << "In ElectronSerialDelegate::ElectronSerialDelegate";
}

ElectronSerialDelegate::~ElectronSerialDelegate() {
  LOG(INFO) << "In ElectronSerialDelegate::~ElectronSerialDelegate";
}

std::unique_ptr<content::SerialChooser> ElectronSerialDelegate::RunChooser(
    content::RenderFrameHost* frame,
    std::vector<blink::mojom::SerialPortFilterPtr> filters,
    content::SerialChooser::Callback callback) {
  LOG(INFO) << "In ElectronSerialDelegate::RunChooser";

  SerialChooserEventHandler* handler = HandlerForFrame(frame);
  if (handler) {
    DeleteHandlerForFrame(frame);
  }
  handler = AddHandlerForFrame(
      frame, std::make_unique<SerialChooserController>(
                 frame, std::move(filters), std::move(callback)));
  return std::make_unique<SerialChooser>(handler->MakeCloseClosure());
}

bool ElectronSerialDelegate::CanRequestPortPermission(
    content::RenderFrameHost* frame) {
  return true;
}

bool ElectronSerialDelegate::HasPortPermission(
    content::RenderFrameHost* frame,
    const device::mojom::SerialPortInfo& port) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(frame);
  auto* browser_context = web_contents->GetBrowserContext();
  auto* chooser_context =
      SerialChooserContextFactory::GetForBrowserContext(browser_context);
  return chooser_context->HasPortPermission(
      frame->GetLastCommittedOrigin(),
      web_contents->GetMainFrame()->GetLastCommittedOrigin(), port);
}

device::mojom::SerialPortManager* ElectronSerialDelegate::GetPortManager(
    content::RenderFrameHost* frame) {
  return GetChooserContext(frame)->GetPortManager();
}

void ElectronSerialDelegate::AddObserver(content::RenderFrameHost* frame,
                                         Observer* observer) {
  return GetChooserContext(frame)->AddPortObserver(observer);
}

void ElectronSerialDelegate::RemoveObserver(content::RenderFrameHost* frame,
                                            Observer* observer) {
  return GetChooserContext(frame)->RemovePortObserver(observer);
}

SerialChooserEventHandler* ElectronSerialDelegate::HandlerForFrame(
    content::RenderFrameHost* render_frame_host) {
  auto mapping = event_handler_map_.find(render_frame_host);
  return mapping == event_handler_map_.end() ? nullptr : mapping->second.get();
}

SerialChooserEventHandler* ElectronSerialDelegate::AddHandlerForFrame(
    content::RenderFrameHost* render_frame_host,
    std::unique_ptr<SerialChooserController> chooser_controller) {
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  auto event_handler = std::make_unique<SerialChooserEventHandler>(
      std::move(chooser_controller), web_contents, weak_factory_.GetWeakPtr());
  event_handler_map_.insert(
      std::make_pair(render_frame_host, std::move(event_handler)));
  return HandlerForFrame(render_frame_host);
}

void ElectronSerialDelegate::DeleteHandlerForFrame(
    content::RenderFrameHost* render_frame_host) {
  LOG(INFO) << "In ElectronSerialDelegate::DeleteHandlerForFrame";
  event_handler_map_.erase(render_frame_host);
}

}  // namespace electron
