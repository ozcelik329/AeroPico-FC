// Apple-style spring presets (WWDC "Designing Fluid Interfaces").
// Two designer-friendly params instead of mass/stiffness/damping:
//   bounce   — overshoot. 0 = critically damped (no bounce), higher = bouncier.
//   duration — settle time in seconds. Not a fixed-duration animation; this is
//              where the spring is *tuned* to land, motion can still be redirected.

// Default UI spring — graceful, non-distracting, no overshoot.
// Use for menus, panels, modals, anything that isn't a direct continuation
// of a flick/drag gesture.
export const springDefault = { type: "spring", bounce: 0, duration: 0.4 };

// Snappier variant for small/local UI (chips, toggles, tooltips).
export const springSnappy = { type: "spring", bounce: 0, duration: 0.3 };

// Momentum spring — slight bounce, reserved for interactions that carried
// physical momentum into the release (a drag, a flick, a throw).
export const springMomentum = { type: "spring", bounce: 0.2, duration: 0.4 };

// Drawer / sheet spring — matches Apple's shipped drawer values.
export const springSheet = { type: "spring", bounce: 0.15, duration: 0.35 };

// Respect the user's OS-level motion preference. Falls back to a short
// cross-fade instead of no animation at all — reduced motion still needs
// *some* feedback, just non-vestibular.
export const prefersReducedMotion = () =>
  typeof window !== "undefined" && window.matchMedia("(prefers-reduced-motion: reduce)").matches;

export const reducedMotionFallback = { duration: 0.15, ease: "easeOut" };

// Pick the right transition for the current environment in one call.
export function spring(preset = springDefault) {
  return prefersReducedMotion() ? reducedMotionFallback : preset;
}

// Instant, continuous press feedback (§1 Response, §10 Gesture design).
// Scale down on pointer-down, not on release — motion's whileTap already
// triggers on pointerdown, so this is just the shared shape for consistency.
export const pressFeedback = {
  whileTap: { scale: 0.96 },
  transition: springSnappy,
};
