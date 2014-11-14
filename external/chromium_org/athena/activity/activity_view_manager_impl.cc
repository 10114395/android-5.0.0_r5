// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "athena/activity/public/activity_view_manager.h"

#include <algorithm>
#include <map>

#include "athena/activity/public/activity.h"
#include "athena/activity/public/activity_view_model.h"
#include "athena/screen/public/screen_manager.h"
#include "ui/aura/window.h"
#include "ui/views/background.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/layout_manager.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace athena {
namespace {

class ActivityWidget : public views::LayoutManager {
 public:
  explicit ActivityWidget(Activity* activity)
      : activity_(activity),
        container_(NULL),
        title_(NULL),
        content_(NULL),
        widget_(NULL) {
    container_ = new views::View;

    title_ = new views::Label();
    title_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    const gfx::FontList& font_list = title_->font_list();
    title_->SetFontList(font_list.Derive(1, gfx::Font::BOLD));
    title_->SetEnabledColor(SK_ColorBLACK);
    container_->AddChildView(title_);
    container_->SetLayoutManager(this);
    content_ = activity->GetActivityViewModel()->GetContentsView();
    container_->AddChildView(content_);

    widget_ = new views::Widget;
    views::Widget::InitParams params(
        views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
    params.context = ScreenManager::Get()->GetContext();
    params.delegate = NULL;
    params.activatable = views::Widget::InitParams::ACTIVATABLE_YES;
    widget_->Init(params);
    widget_->SetContentsView(container_);

    activity_->GetActivityViewModel()->Init();
  }

  virtual ~ActivityWidget() {}

  void Show() {
    Update();
    widget_->Show();
  }

  void Update() {
    title_->SetText(activity_->GetActivityViewModel()->GetTitle());
    SkColor bgcolor =
        activity_->GetActivityViewModel()->GetRepresentativeColor();
    title_->set_background(views::Background::CreateSolidBackground(bgcolor));
    title_->SetBackgroundColor(bgcolor);
  }

 private:
  // views::LayoutManager:
  virtual void Layout(views::View* host) OVERRIDE {
    CHECK_EQ(container_, host);
    const gfx::Rect& content_bounds = host->bounds();
    const int kTitleHeight = 25;
    title_->SetBounds(0, 0, content_bounds.width(), kTitleHeight);
    content_->SetBounds(0,
                        kTitleHeight,
                        content_bounds.width(),
                        content_bounds.height() - kTitleHeight);
  }

  virtual gfx::Size GetPreferredSize(const views::View* host) const OVERRIDE {
    CHECK_EQ(container_, host);
    gfx::Size size;
    gfx::Size label_size = title_->GetPreferredSize();
    gfx::Size content_size = content_->GetPreferredSize();

    size.set_width(std::max(label_size.width(), content_size.width()));
    size.set_height(label_size.height() + content_size.height());
    return size;
  }

  Activity* activity_;
  views::View* container_;
  views::Label* title_;
  views::View* content_;
  views::Widget* widget_;

  DISALLOW_COPY_AND_ASSIGN(ActivityWidget);
};

ActivityViewManager* instance = NULL;

class ActivityViewManagerImpl : public ActivityViewManager {
 public:
  ActivityViewManagerImpl() {
    CHECK(!instance);
    instance = this;
  }
  virtual ~ActivityViewManagerImpl() {
    CHECK_EQ(this, instance);
    instance = NULL;
  }

  // ActivityViewManager:
  virtual void AddActivity(Activity* activity) OVERRIDE {
    CHECK(activity_widgets_.end() == activity_widgets_.find(activity));
    ActivityWidget* container = new ActivityWidget(activity);
    activity_widgets_[activity] = container;
    container->Show();
  }

  virtual void RemoveActivity(Activity* activity) OVERRIDE {
    std::map<Activity*, ActivityWidget*>::iterator find =
        activity_widgets_.find(activity);
    if (find != activity_widgets_.end())
      activity_widgets_.erase(activity);
  }

  virtual void UpdateActivity(Activity* activity) OVERRIDE {
    std::map<Activity*, ActivityWidget*>::iterator find =
        activity_widgets_.find(activity);
    if (find != activity_widgets_.end())
      find->second->Update();
  }

 private:
  std::map<Activity*, ActivityWidget*> activity_widgets_;

  DISALLOW_COPY_AND_ASSIGN(ActivityViewManagerImpl);
};

}  // namespace

// static
ActivityViewManager* ActivityViewManager::Create() {
  new ActivityViewManagerImpl();
  CHECK(instance);
  return instance;
}

ActivityViewManager* ActivityViewManager::Get() {
  return instance;
}

void ActivityViewManager::Shutdown() {
  CHECK(instance);
  delete instance;
}

}  // namespace athena
