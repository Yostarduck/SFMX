#include "input/Interaction.h"

namespace sfmx
{

namespace {

// PassThrough: actuated => performed every frame; release => canceled once.
ActionPhase
stepPassThrough(Interaction& it, bool actuated) {
  if (actuated) {
    it.m_wasActuated = true;
    return ActionPhase::kPerformed;
  }
  if (it.m_wasActuated) {
    it.m_wasActuated = false;
    return ActionPhase::kCanceled;
  }
  return ActionPhase::kWaiting;
}

// Press: performed once on the press edge, canceled on release.
ActionPhase
stepPress(Interaction& it, bool actuated) {
  if (actuated) {
    if (!it.m_wasActuated) {
      it.m_wasActuated = true;
      return ActionPhase::kPerformed;
    }
    return ActionPhase::kStarted;
  }
  if (it.m_wasActuated) {
    it.m_wasActuated = false;
    return ActionPhase::kCanceled;
  }
  return ActionPhase::kWaiting;
}

// Hold: started on press; performed once after m_duration; canceled on release.
ActionPhase
stepHold(Interaction& it, bool actuated, float dt) {
  if (actuated) {
    if (!it.m_wasActuated) {
      it.m_wasActuated = true;
      it.m_elapsed = 0.f;
      it.m_flag = false;  // not yet fired
      return ActionPhase::kStarted;
    }
    it.m_elapsed += dt;
    if (!it.m_flag && it.m_elapsed >= it.m_duration) {
      it.m_flag = true;
      return ActionPhase::kPerformed;
    }
    return ActionPhase::kStarted;
  }
  if (it.m_wasActuated) {
    it.m_wasActuated = false;
    it.m_flag = false;
    return ActionPhase::kCanceled;
  }
  return ActionPhase::kWaiting;
}

// Tap: started on press; performed if released within m_duration; canceled if
// held too long.
ActionPhase
stepTap(Interaction& it, bool actuated, float dt) {
  if (actuated) {
    if (!it.m_wasActuated) {
      it.m_wasActuated = true;
      it.m_elapsed = 0.f;
      it.m_flag = false;  // not failed
      return ActionPhase::kStarted;
    }
    it.m_elapsed += dt;
    if (!it.m_flag && it.m_elapsed > it.m_duration) {
      it.m_flag = true;  // held too long: tap failed
      return ActionPhase::kCanceled;
    }
    return it.m_flag ? ActionPhase::kWaiting : ActionPhase::kStarted;
  }
  if (it.m_wasActuated) {
    it.m_wasActuated = false;
    const bool succeeded = !it.m_flag && it.m_elapsed <= it.m_duration;
    it.m_flag = false;
    return succeeded ? ActionPhase::kPerformed : ActionPhase::kWaiting;
  }
  return ActionPhase::kWaiting;
}

// MultiTap: performed after m_tapCount presses, each within m_tapSpacing.
ActionPhase
stepMultiTap(Interaction& it, bool actuated, float dt) {
  // Press edge.
  if (actuated && !it.m_wasActuated) {
    it.m_wasActuated = true;
    it.m_elapsed = 0.f;
    return ActionPhase::kStarted;
  }
  // Release edge.
  if (!actuated && it.m_wasActuated) {
    it.m_wasActuated = false;
    it.m_elapsed = 0.f;
    ++it.m_progress;
    if (it.m_progress >= it.m_tapCount) {
      it.m_progress = 0;
      return ActionPhase::kPerformed;
    }
    return ActionPhase::kStarted;
  }
  // No edge: time the gap between taps.
  it.m_elapsed += dt;
  if (it.m_progress > 0 && it.m_elapsed > it.m_tapSpacing) {
    it.m_progress = 0;
    return ActionPhase::kCanceled;
  }
  return it.m_wasActuated ? ActionPhase::kStarted : ActionPhase::kWaiting;
}

} // namespace

ActionPhase
Interaction::step(bool actuated, float deltaTime) {
  switch (m_type) {
    case InteractionType::kPress:    return stepPress(*this, actuated);
    case InteractionType::kHold:     return stepHold(*this, actuated, deltaTime);
    case InteractionType::kTap:      return stepTap(*this, actuated, deltaTime);
    case InteractionType::kMultiTap: return stepMultiTap(*this, actuated, deltaTime);
    case InteractionType::kPassThrough:
    default:                         return stepPassThrough(*this, actuated);
  }
}

} // namespace sfmx
