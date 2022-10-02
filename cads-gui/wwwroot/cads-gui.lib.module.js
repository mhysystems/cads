export function beforeStart(options, extensions) {
  console.log("beforeStart");
}

export function afterStarted(blazor) {
  console.log("afterStarted");
}

export function getBoundingClientRect(element) {
  const o = element.getBoundingClientRect();
  return {x:o.left, y:o.top, right:o.right, bottom:o.bottom};
}

export function getBoundingClientRect2(elementId) {
  const o = document.querySelector(elementId).getBoundingClientRect();
  return {x:o.left, y:o.top, right:o.right, bottom:o.bottom};
}