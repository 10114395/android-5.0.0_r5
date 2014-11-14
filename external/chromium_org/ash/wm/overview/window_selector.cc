// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/window_selector.h"

#include <algorithm>

#include "ash/accessibility_delegate.h"
#include "ash/ash_switches.h"
#include "ash/metrics/user_metrics_recorder.h"
#include "ash/root_window_controller.h"
#include "ash/shell.h"
#include "ash/shell_window_ids.h"
#include "ash/switchable_windows.h"
#include "ash/wm/overview/scoped_transform_overview_window.h"
#include "ash/wm/overview/window_grid.h"
#include "ash/wm/overview/window_selector_delegate.h"
#include "ash/wm/overview/window_selector_item.h"
#include "ash/wm/window_state.h"
#include "base/auto_reset.h"
#include "base/metrics/histogram.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_observer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/events/event.h"
#include "ui/gfx/screen.h"
#include "ui/wm/core/window_util.h"
#include "ui/wm/public/activation_client.h"

namespace ash {

namespace {

// A comparator for locating a grid with a given root window.
struct RootWindowGridComparator
    : public std::unary_function<WindowGrid*, bool> {
  explicit RootWindowGridComparator(const aura::Window* root_window)
      : root_window_(root_window) {
  }

  bool operator()(WindowGrid* grid) const {
    return (grid->root_window() == root_window_);
  }

  const aura::Window* root_window_;
};

// A comparator for locating a selectable window given a targeted window.
struct WindowSelectorItemTargetComparator
    : public std::unary_function<WindowSelectorItem*, bool> {
  explicit WindowSelectorItemTargetComparator(const aura::Window* target_window)
      : target(target_window) {
  }

  bool operator()(WindowSelectorItem* window) const {
    return window->Contains(target);
  }

  const aura::Window* target;
};

// A comparator for locating a selector item for a given root.
struct WindowSelectorItemForRoot
    : public std::unary_function<WindowSelectorItem*, bool> {
  explicit WindowSelectorItemForRoot(const aura::Window* root)
      : root_window(root) {
  }

  bool operator()(WindowSelectorItem* item) const {
    return item->GetRootWindow() == root_window;
  }

  const aura::Window* root_window;
};

// Triggers a shelf visibility update on all root window controllers.
void UpdateShelfVisibility() {
  Shell::RootWindowControllerList root_window_controllers =
      Shell::GetInstance()->GetAllRootWindowControllers();
  for (Shell::RootWindowControllerList::iterator iter =
          root_window_controllers.begin();
       iter != root_window_controllers.end(); ++iter) {
    (*iter)->UpdateShelfVisibility();
  }
}

}  // namespace

WindowSelector::WindowSelector(const WindowList& windows,
                               WindowSelectorDelegate* delegate)
    : delegate_(delegate),
      restore_focus_window_(aura::client::GetFocusClient(
          Shell::GetPrimaryRootWindow())->GetFocusedWindow()),
      ignore_activations_(false),
      selected_grid_index_(0),
      overview_start_time_(base::Time::Now()),
      num_key_presses_(0),
      num_items_(0) {
  DCHECK(delegate_);
  Shell* shell = Shell::GetInstance();
  shell->OnOverviewModeStarting();

  if (restore_focus_window_)
    restore_focus_window_->AddObserver(this);

  const aura::Window::Windows root_windows = Shell::GetAllRootWindows();
  for (aura::Window::Windows::const_iterator iter = root_windows.begin();
       iter != root_windows.end(); iter++) {
    // Observed switchable containers for newly created windows on all root
    // windows.
    for (size_t i = 0; i < kSwitchableWindowContainerIdsLength; ++i) {
      aura::Window* container = Shell::GetContainer(*iter,
          kSwitchableWindowContainerIds[i]);
      container->AddObserver(this);
      observed_windows_.insert(container);
    }
    scoped_ptr<WindowGrid> grid(new WindowGrid(*iter, windows, this));
    if (grid->empty())
      continue;
    num_items_ += grid->size();
    grid_list_.push_back(grid.release());
  }

  // Do not call PrepareForOverview until all items are added to window_list_ as
  // we don't want to cause any window updates until all windows in overview
  // are observed. See http://crbug.com/384495.
  for (ScopedVector<WindowGrid>::iterator iter = grid_list_.begin();
       iter != grid_list_.end(); ++iter) {
    (*iter)->PrepareForOverview();
    (*iter)->PositionWindows(true);
  }

  DCHECK(!grid_list_.empty());
  UMA_HISTOGRAM_COUNTS_100("Ash.WindowSelector.Items", num_items_);

  shell->activation_client()->AddObserver(this);

  // Remove focus from active window before entering overview.
  aura::client::GetFocusClient(
      Shell::GetPrimaryRootWindow())->FocusWindow(NULL);

  shell->PrependPreTargetHandler(this);
  shell->GetScreen()->AddObserver(this);
  shell->metrics()->RecordUserMetricsAction(UMA_WINDOW_OVERVIEW);
  HideAndTrackNonOverviewWindows();
  // Send an a11y alert.
  shell->accessibility_delegate()->TriggerAccessibilityAlert(
      A11Y_ALERT_WINDOW_OVERVIEW_MODE_ENTERED);

  UpdateShelfVisibility();
}

WindowSelector::~WindowSelector() {
  ash::Shell* shell = ash::Shell::GetInstance();

  ResetFocusRestoreWindow(true);
  for (std::set<aura::Window*>::iterator iter = observed_windows_.begin();
       iter != observed_windows_.end(); ++iter) {
    (*iter)->RemoveObserver(this);
  }
  shell->activation_client()->RemoveObserver(this);
  aura::Window::Windows root_windows = Shell::GetAllRootWindows();

  const aura::WindowTracker::Windows hidden_windows(hidden_windows_.windows());
  for (aura::WindowTracker::Windows::const_iterator iter =
       hidden_windows.begin(); iter != hidden_windows.end(); ++iter) {
    ui::ScopedLayerAnimationSettings settings(
        (*iter)->layer()->GetAnimator());
    settings.SetTransitionDuration(base::TimeDelta::FromMilliseconds(
        ScopedTransformOverviewWindow::kTransitionMilliseconds));
    settings.SetPreemptionStrategy(
        ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET);
    (*iter)->layer()->SetOpacity(1);
    (*iter)->Show();
  }

  shell->RemovePreTargetHandler(this);
  shell->GetScreen()->RemoveObserver(this);

  size_t remaining_items = 0;
  for (ScopedVector<WindowGrid>::iterator iter = grid_list_.begin();
      iter != grid_list_.end(); iter++) {
    remaining_items += (*iter)->size();
  }

  DCHECK(num_items_ >= remaining_items);
  UMA_HISTOGRAM_COUNTS_100("Ash.WindowSelector.OverviewClosedItems",
                           num_items_ - remaining_items);
  UMA_HISTOGRAM_MEDIUM_TIMES("Ash.WindowSelector.TimeInOverview",
                             base::Time::Now() - overview_start_time_);

  // TODO(nsatragno): Change this to OnOverviewModeEnded and move it to when
  // everything is done.
  shell->OnOverviewModeEnding();

  // Clearing the window list resets the ignored_by_shelf flag on the windows.
  grid_list_.clear();
  UpdateShelfVisibility();
}

void WindowSelector::CancelSelection() {
  delegate_->OnSelectionEnded();
}

void WindowSelector::OnGridEmpty(WindowGrid* grid) {
  ScopedVector<WindowGrid>::iterator iter =
      std::find(grid_list_.begin(), grid_list_.end(), grid);
  DCHECK(iter != grid_list_.end());
  grid_list_.erase(iter);
  // TODO(nsatragno): Use the previous index for more than two displays.
  selected_grid_index_ = 0;
  if (grid_list_.empty())
    CancelSelection();
}

void WindowSelector::OnKeyEvent(ui::KeyEvent* event) {
  if (event->type() != ui::ET_KEY_PRESSED)
    return;

  switch (event->key_code()) {
    case ui::VKEY_ESCAPE:
      CancelSelection();
      break;
    case ui::VKEY_UP:
      num_key_presses_++;
      Move(WindowSelector::UP);
      break;
    case ui::VKEY_DOWN:
      num_key_presses_++;
      Move(WindowSelector::DOWN);
      break;
    case ui::VKEY_RIGHT:
      num_key_presses_++;
      Move(WindowSelector::RIGHT);
      break;
    case ui::VKEY_LEFT:
      num_key_presses_++;
      Move(WindowSelector::LEFT);
      break;
    case ui::VKEY_RETURN:
      // Ignore if no item is selected.
      if (!grid_list_[selected_grid_index_]->is_selecting())
        return;
      UMA_HISTOGRAM_COUNTS_100("Ash.WindowSelector.ArrowKeyPresses",
                               num_key_presses_);
      UMA_HISTOGRAM_CUSTOM_COUNTS(
          "Ash.WindowSelector.KeyPressesOverItemsRatio",
          (num_key_presses_ * 100) / num_items_, 1, 300, 30);
      Shell::GetInstance()->metrics()->RecordUserMetricsAction(
          UMA_WINDOW_OVERVIEW_ENTER_KEY);
      wm::GetWindowState(grid_list_[selected_grid_index_]->
                         SelectedWindow()->SelectionWindow())->Activate();
      break;
    default:
      // Not a key we are interested in.
      return;
  }
  event->StopPropagation();
}

void WindowSelector::OnDisplayAdded(const gfx::Display& display) {
}

void WindowSelector::OnDisplayRemoved(const gfx::Display& display) {
  // TODO(nsatragno): Keep window selection active on remaining displays.
  CancelSelection();
}

void WindowSelector::OnDisplayMetricsChanged(const gfx::Display& display,
                                             uint32_t metrics) {
  PositionWindows(/* animate */ false);
}

void WindowSelector::OnWindowAdded(aura::Window* new_window) {
  if (new_window->type() != ui::wm::WINDOW_TYPE_NORMAL &&
      new_window->type() != ui::wm::WINDOW_TYPE_PANEL) {
    return;
  }

  for (size_t i = 0; i < kSwitchableWindowContainerIdsLength; ++i) {
    if (new_window->parent()->id() == kSwitchableWindowContainerIds[i] &&
        !::wm::GetTransientParent(new_window)) {
      // The new window is in one of the switchable containers, abort overview.
      CancelSelection();
      return;
    }
  }
}

void WindowSelector::OnWindowDestroying(aura::Window* window) {
  window->RemoveObserver(this);
  observed_windows_.erase(window);
  if (window == restore_focus_window_)
    restore_focus_window_ = NULL;
}

void WindowSelector::OnWindowActivated(aura::Window* gained_active,
                                       aura::Window* lost_active) {
  if (ignore_activations_ || !gained_active)
    return;

  ScopedVector<WindowGrid>::iterator grid =
      std::find_if(grid_list_.begin(), grid_list_.end(),
                   RootWindowGridComparator(gained_active->GetRootWindow()));
  if (grid == grid_list_.end())
    return;
  const std::vector<WindowSelectorItem*> windows = (*grid)->window_list();

  ScopedVector<WindowSelectorItem>::const_iterator iter = std::find_if(
      windows.begin(), windows.end(),
      WindowSelectorItemTargetComparator(gained_active));

  if (iter != windows.end())
    (*iter)->RestoreWindowOnExit(gained_active);

  // Don't restore focus on exit if a window was just activated.
  ResetFocusRestoreWindow(false);
  CancelSelection();
}

void WindowSelector::OnAttemptToReactivateWindow(aura::Window* request_active,
                                                 aura::Window* actual_active) {
  OnWindowActivated(request_active, actual_active);
}

void WindowSelector::PositionWindows(bool animate) {
  for (ScopedVector<WindowGrid>::iterator iter = grid_list_.begin();
      iter != grid_list_.end(); iter++) {
    (*iter)->PositionWindows(animate);
  }
}

void WindowSelector::HideAndTrackNonOverviewWindows() {
  // Add the windows to hidden_windows first so that if any are destroyed
  // while hiding them they are tracked.
  for (ScopedVector<WindowGrid>::iterator grid_iter = grid_list_.begin();
      grid_iter != grid_list_.end(); ++grid_iter) {
    for (size_t i = 0; i < kSwitchableWindowContainerIdsLength; ++i) {
      const aura::Window* container =
          Shell::GetContainer((*grid_iter)->root_window(),
                              kSwitchableWindowContainerIds[i]);
      for (aura::Window::Windows::const_iterator iter =
           container->children().begin(); iter != container->children().end();
           ++iter) {
        if (!(*iter)->IsVisible() || (*grid_iter)->Contains(*iter))
          continue;
        hidden_windows_.Add(*iter);
      }
    }
  }

  // Copy the window list as it can change during iteration.
  const aura::WindowTracker::Windows hidden_windows(hidden_windows_.windows());
  for (aura::WindowTracker::Windows::const_iterator iter =
       hidden_windows.begin(); iter != hidden_windows.end(); ++iter) {
    if (!hidden_windows_.Contains(*iter))
      continue;
    ui::ScopedLayerAnimationSettings settings(
        (*iter)->layer()->GetAnimator());
    settings.SetTransitionDuration(base::TimeDelta::FromMilliseconds(
        ScopedTransformOverviewWindow::kTransitionMilliseconds));
    settings.SetPreemptionStrategy(
        ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET);
    (*iter)->Hide();
    // Hiding the window can result in it being destroyed.
    if (!hidden_windows_.Contains(*iter))
      continue;
    (*iter)->layer()->SetOpacity(0);
  }
}

void WindowSelector::ResetFocusRestoreWindow(bool focus) {
  if (!restore_focus_window_)
    return;
  if (focus) {
    base::AutoReset<bool> restoring_focus(&ignore_activations_, true);
    restore_focus_window_->Focus();
  }
  // If the window is in the observed_windows_ list it needs to continue to be
  // observed.
  if (observed_windows_.find(restore_focus_window_) ==
          observed_windows_.end()) {
    restore_focus_window_->RemoveObserver(this);
  }
  restore_focus_window_ = NULL;
}

void WindowSelector::Move(Direction direction) {
  bool overflowed = grid_list_[selected_grid_index_]->Move(direction);
  if (overflowed) {
    // The grid reported that the movement command corresponds to the next
    // root window, identify it and call Move() on it to initialize the
    // selection widget.
    // TODO(nsatragno): If there are more than two monitors, move between grids
    // in the requested direction.
    selected_grid_index_ = (selected_grid_index_ + 1) % grid_list_.size();
    grid_list_[selected_grid_index_]->Move(direction);
  }
}

}  // namespace ash
