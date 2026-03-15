import { useReducer, useCallback } from "react";

export interface AnimatedPresenceResult {
  shouldRender: boolean;
  className: string;
  onAnimationEnd: (() => void) | undefined;
}

type State = { phase: "visible" | "exiting" | "hidden" };
type Action = { type: "show" } | { type: "hide" } | { type: "exited" };

function reducer(_state: State, action: Action): State {
  switch (action.type) {
    case "show":
      return { phase: "visible" };
    case "hide":
      return { phase: "exiting" };
    case "exited":
      return { phase: "hidden" };
  }
}

export function useAnimatedPresence(
  visible: boolean,
  enterClass: string,
  exitClass: string,
): AnimatedPresenceResult {
  const [state, dispatch] = useReducer(reducer, {
    phase: visible ? "visible" : "hidden",
  });

  // Sync visibility changes
  if (visible && state.phase !== "visible") {
    dispatch({ type: "show" });
  } else if (!visible && state.phase === "visible") {
    dispatch({ type: "hide" });
  }

  const onAnimationEnd = useCallback(() => {
    dispatch({ type: "exited" });
  }, []);

  const shouldRender = state.phase !== "hidden";
  const className = state.phase === "exiting" ? exitClass : enterClass;

  return {
    shouldRender,
    className: shouldRender ? className : "",
    onAnimationEnd: state.phase === "exiting" ? onAnimationEnd : undefined,
  };
}
